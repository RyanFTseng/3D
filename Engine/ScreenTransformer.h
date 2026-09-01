#pragma once
#include "Vec3.h"
#include "Graphics.h"


class ScreenTransformer
{
public:
	ScreenTransformer()
		:
		xFactor(float(Graphics::ScreenWidth) / 2.0f),
		yFactor(float(Graphics::ScreenHeight) / 2.0f)
	{}
	template<class Vertex>
	Vertex& Transform(Vertex& v) const
	{
		//reciprocal of z
		const float zInv = 1.0f / v.pos.z;

		//divide all components of the vertex by z
		v *= zInv;

		//adjust x,y from perspective normalized space to screen space after perspective divide
		v.pos.x = (v.pos.x + 1.0f) * xFactor;
		v.pos.y = (-v.pos.y + 1.0f) * yFactor;
		//temporarily store z inverse in v.pos.z to recover z later in pipeline
		v.pos.z = zInv;
		return v;
	}

	template<class Vertex>
	Vertex GetTransformed(const Vertex& v) const
	{
		return Transform(Vertex(v));
	}
private:
	float xFactor;
	float yFactor;
};