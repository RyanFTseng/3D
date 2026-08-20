#pragma once

#include "ScreenTransformer.h"
#include "ChiliWin.h"
#include "Graphics.h"
#include "Triangle.h"
#include "IndexedTriangleList.h"
#include "Mat3.h"
#include <algorithm>


//triangle pipeline
class Pipeline
{
public:
	class Vertex
	{
	public:
		Vertex() = default;
		Vertex( const Vec3& pos)
			:
			pos(pos)
		{}
		Vertex(const Vec3& pos, const Vertex& src)
			:
			pos(pos),
			t(src.t)
		{}
		Vertex(const Vec3& pos, const Vec2& t)
			:
			pos(pos),
			t(t)
		{}
		Vertex& operator+=(const Vertex& rhs)
		{
			pos += rhs.pos;
			t += rhs.t;
			return *this;
		}
		Vertex& operator+(const Vertex& rhs) const
		{
			return Vertex(*this) += rhs;
		}
		Vertex& operator-=(const Vertex& rhs)
		{
			pos -= rhs.pos;
			t -= rhs.t;
			return *this;
		}
		Vertex& operator-(const Vertex& rhs) const
		{
			return Vertex(*this) -= rhs;
		}
		Vertex& operator*=( float rhs)
		{
			pos *= rhs;
			t *= rhs;
			return *this;
		}
		Vertex& operator*(float rhs) const
		{
			return Vertex(*this) *= rhs;
		}
		Vertex& operator/=(float rhs)
		{
			pos /= rhs;
			t /= rhs;
			return *this;
		}
		Vertex& operator/(float rhs) const
		{
			return Vertex(*this) /= rhs;
		}


		Vec3 pos;
		Vec2 t;
	};

public:
	Pipeline(Graphics& gfx)
		:
		gfx(gfx)
	{}
	void Draw(IndexedTriangleList<Vertex>& triList)
	{
		ProcessVertices(triList.vertices, triList.indices);
	}
	void BindRotation(const Mat3& rotation_in)
	{
		rotation = rotation_in;
	}
	void BindRotation(const Vec3& translation_in)
	{
		translation = translation_in;
	}
	void BindTexture(const std::wstring& filename)
	{
		pTex = std::make_unique<Surface>(Surface::FromFile(filename));
	}

private:
	//vertex processing
	//transforms vertices and passes vtx and idx lists to triangle assembler
	void ProcessVertices(const std::vector<Vertex>& vertices, const std::vector<size_t>& indices)
	{
		//create vertex vector for vs output
		std::vector<Vertex> verticesOut;

		//transform vertices w matrix + vector
		for (const auto& v : vertices)
		{
			verticesOut.emplace_back(v.pos * rotation + translation, v.t);
		}

		//assemble triangles using indices and vertices
		AssembleTriangles(verticesOut, indices);
	}

	//triangle assembly funcion
	//assembles indexed vertex stream into triangles and passes them to post process
	//backface culling
	void AssembleTriangles(const std::vector<Vertex>& vertices, const std::vector<size_t>& indices)
	{
		//assemble triangle in stream and process
		for (size_t i = 0, end = indices.size() / 3; i < end; i++)
		{
			//determine triangle face using indices
			const auto& v0 = vertices[indices[i * 3]];
			const auto& v1 = vertices[indices[i * 3 + 1]];
			const auto& v2 = vertices[indices[i * 3 + 1]];

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
	void ProcessTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2)
	{

	}


private:
	Graphics& gfx;
	ScreenTransformer pst;
	Mat3 rotation;
	Vec3 translation;
	std::unique_ptr<Surface> pTex;
};