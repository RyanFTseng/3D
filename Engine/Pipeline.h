#pragma once

#include <algorithm>
#include "ScreenTransformer.h"
#include "ChiliWin.h"
#include "Graphics.h"
#include "Triangle.h"
#include "IndexedTriangleList.h"
#include "Mat3.h"
#include "ZBuffer.h"



//triangle pipeline
//pixel shading stage
template <class Effect> 
class Pipeline
{
public:
	//input vertex type
	typedef typename Effect::Vertex Vertex;
	//output vertex type
	typedef typename Effect::VertexShader::Output VSOut;

public:
	Pipeline( Graphics& gfx )
		:
		gfx( gfx ),
		zb(gfx.ScreenWidth, gfx.ScreenHeight)

	{}
	void Draw( IndexedTriangleList<Vertex>& triList )
	{
		ProcessVertices( triList.vertices,triList.indices );
	}
	void BeginFrame()
	{
		zb.Clear();
	}

private:
	//vertex processing
	//transforms vertices using vertex shader and passes vtx and idx lists to triangle assembler
	void ProcessVertices(const std::vector<Vertex>& vertices, const std::vector<size_t>& indices)
	{
		// create vertex vector for vs output
		std::vector<VSOut> verticesOut(vertices.size());

		// transform vertices using vertex shader
		//run effect.vs functor on all vertices and store in verticesOut
		std::transform(vertices.begin(), vertices.end(), verticesOut.begin(), effect.vs);
		
		// assemble triangles from stream of indices and vertices
		AssembleTriangles(verticesOut, indices);
	}

	//triangle assembly funcion
	//assembles indexed vertex stream into triangles and passes them to post process
	//backface culling
	void AssembleTriangles(const std::vector<VSOut>& vertices, const std::vector<size_t>& indices)
	{
		//assemble triangle in stream and process
		for (size_t i = 0, end = indices.size() / 3; i < end; i++)
		{
			//determine triangle face using indices
			const auto& v0 = vertices[indices[i * 3]];
			const auto& v1 = vertices[indices[i * 3 + 1]];
			const auto& v2 = vertices[indices[i * 3 + 2]];

			//cull backfacing triangles using cross product(%)
			if ((v1.pos - v0.pos) % (v2.pos - v0.pos) *v0.pos <= 0.0f)
			{
				//process triangle using vertices
				ProcessTriangle(v0, v1, v2);
			}
		}

	}

	//triangle processing function
	//takes 3 vertices
	//sends genertated triangle to post-processing
	void ProcessTriangle(const VSOut& v0, const VSOut& v1, const VSOut& v2)
	{
		//generate triangle from 3 vertices using geometry shader
		//and send to post-processing
		PostProcessTriangleVertices(Triangle<VSOut>{v0, v1, v2});
	}


	//vertex postprocessing function	
	//perform perspective and viewport transformations
	void PostProcessTriangleVertices(Triangle<VSOut>& triangle)
	{
		//perspective divide and screen transform for all 3 vertices
		st.Transform(triangle.v0);
		st.Transform(triangle.v1);
		st.Transform(triangle.v2);

		//draw the triangle
		DrawTriangle(triangle);
	}

	//triangle rasterization function
	//sorts verts -> determines cases -> splits to flat top/flat bot -> dispatch to flat tri funcs
	void DrawTriangle(const Triangle<VSOut>& triangle)
	{
		// using pointers so we can swap (for sorting purposes)
		const VSOut* pv0 = &triangle.v0;
		const VSOut* pv1 = &triangle.v1;
		const VSOut* pv2 = &triangle.v2;

		// sorting vertices by y
		if (pv1->pos.y < pv0->pos.y) std::swap(pv0, pv1);
		if (pv2->pos.y < pv1->pos.y) std::swap(pv1, pv2);
		if (pv1->pos.y < pv0->pos.y) std::swap(pv0, pv1);

		if (pv0->pos.y == pv1->pos.y) // natural flat top
		{
			// sorting top vertices by x
			if (pv1->pos.x < pv0->pos.x) std::swap(pv0, pv1);

			DrawFlatTopTriangle(*pv0, *pv1, *pv2);
		}
		else if (pv1->pos.y == pv2->pos.y) // natural flat bottom
		{
			// sorting bottom vertices by x
			if (pv2->pos.x < pv1->pos.x) std::swap(pv1, pv2);

			DrawFlatBottomTriangle(*pv0, *pv1, *pv2);
		}
		else // general triangle
		{
			// find splitting vertex interpolant
			const float alphaSplit =
				(pv1->pos.y - pv0->pos.y) /
				(pv2->pos.y - pv0->pos.y);
			const auto vi = interpolate(*pv0, *pv2, alphaSplit);

			if (pv1->pos.x < vi.pos.x) // major right
			{
				DrawFlatBottomTriangle(*pv0, *pv1, vi);
				DrawFlatTopTriangle(*pv1, vi, *pv2);
			}
			else // major left
			{
				DrawFlatBottomTriangle(*pv0, vi, *pv1);
				DrawFlatTopTriangle(vi, *pv1, *pv2);
			}
		}
	}
	// does flat *TOP* tri-specific calculations and calls DrawFlatTriangle
	void DrawFlatTopTriangle( const VSOut& it0,
							  const VSOut& it1,
							  const VSOut& it2 )
	{
		// calulcate dVertex / dy
		// change in interpolant for every 1 change in y
		const float delta_y = it2.pos.y - it0.pos.y;
		const auto dit0 = (it2 - it0) / delta_y;
		const auto dit1 = (it2 - it1) / delta_y;

		// create right edge interpolant
		auto itEdge1 = it1;

		// call the flat triangle render routine
		DrawFlatTriangle( it0,it1,it2,dit0,dit1,itEdge1 );
	}
	// does flat *BOTTOM* tri-specific calculations and calls DrawFlatTriangle
	void DrawFlatBottomTriangle(const VSOut& it0,
		const VSOut& it1,
		const VSOut& it2)
	{
		// calulcate dVertex / dy
		// change in interpolant for every 1 change in y
		const float delta_y = it2.pos.y - it0.pos.y;
		const auto dit0 = (it1 - it0) / delta_y;
		const auto dit1 = (it2 - it0) / delta_y;

		// create right edge interpolant
		auto itEdge1 = it0;

		// call the flat triangle render routine
		DrawFlatTriangle(it0, it1, it2, dit0, dit1, itEdge1);
	}
	// does processing common to both flat top and flat bottom tris
	// texture lookup and pixel written here
	//perform depth culling
	void DrawFlatTriangle(const VSOut& it0,
		const VSOut& it1,
		const VSOut& it2,
		const VSOut& dv0,
		const VSOut& dv1,
		VSOut itEdge1)
	{
		// create edge interpolant for left edge (always v0)
		auto itEdge0 = it0;

		// calculate start and end scanlines
		const int yStart = (int)ceil(it0.pos.y - 0.5f);
		const int yEnd = (int)ceil(it2.pos.y - 0.5f); // the scanline AFTER the last line drawn

		// do interpolant prestep
		itEdge0 += dv0 * (float(yStart) + 0.5f - it0.pos.y);
		itEdge1 += dv1 * (float(yStart) + 0.5f - it0.pos.y);


		for (int y = yStart; y < yEnd; y++, itEdge0 += dv0, itEdge1 += dv1)
		{
			// calculate start and end pixels
			const int xStart = (int)ceil(itEdge0.pos.x - 0.5f);
			const int xEnd = (int)ceil(itEdge1.pos.x - 0.5f); // the pixel AFTER the last pixel drawn

			// create scanline interpolant startpoint
			// (some waste for interpolating x,y,z, but makes life easier not having
			//  to split them off, and z will be needed in the future anyways...)
			auto iLine = itEdge0;

			// calculate delta scanline interpolant / dx
			const float dx = itEdge1.pos.x - itEdge0.pos.x;
			const auto diLine = (itEdge1 - iLine) / dx;

			// prestep scanline interpolant
			iLine += diLine * (float(xStart) + 0.5f - itEdge0.pos.x);

			for (int x = xStart; x < xEnd; x++, iLine += diLine)
			{

				//recover interpolated z from interpolated 1/z
				//flip 1/z to get z
				const float z = 1.0f / iLine.pos.z;
				if (zb.TestAndSet(x, y, z))
				{
					//recover interpolated attributes
					//multiply all attributes in vertex by z
					//attr z is currently 1/z
					const auto attr = iLine * z;
					//invoke pixel shader and write color value
					gfx.PutPixel(x, y, effect.ps(attr));
				}
				
			}
		}
	}


	public: 
		Effect effect;
	private:
		Graphics& gfx;
		ZBuffer zb;
		ScreenTransformer st;
};