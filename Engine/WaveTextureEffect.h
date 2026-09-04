#pragma once

#include "Pipeline.h"
#include "Plane.h"
#include <cmath>

// solid color attribute not interpolated
class WaveTextureEffect
{
public:
	// the vertex type that will be input into the pipeline
	class Vertex
	{
	public:
		Vertex() = default;
		Vertex(const Vec3& pos)
			:
			pos(pos)
		{}
		Vertex(const Vec3& pos, const Vertex& src)
			:
			t(src.t),
			pos(pos)
		{}
		Vertex(const Vec3& pos, const Vec2& t)
			:
			t(t),
			pos(pos)
		{}
		Vertex& operator+=(const Vertex& rhs)
		{
			pos += rhs.pos;
			return *this;
		}
		Vertex operator+(const Vertex& rhs) const
		{
			return Vertex(*this) += rhs;
		}
		Vertex& operator-=(const Vertex& rhs)
		{
			pos -= rhs.pos;
			return *this;
		}
		Vertex operator-(const Vertex& rhs) const
		{
			return Vertex(*this) -= rhs;
		}
		Vertex& operator*=(float rhs)
		{
			pos *= rhs;
			return *this;
		}
		Vertex operator*(float rhs) const
		{
			return Vertex(*this) *= rhs;
		}
		Vertex& operator/=(float rhs)
		{
			pos /= rhs;
			return *this;
		}
		Vertex operator/(float rhs) const
		{
			return Vertex(*this) /= rhs;
		}
	public:
		Vec3 pos;
		Vec2 t;
	};
	// default vs rotates and translates vertices
	// does not touch attributes
	class VertexShader
	{
	public:
		typedef Vertex Output;
	public:
		void BindRotation(const Mat3& rotation_in)
		{
			rotation = rotation_in;
		}
		void BindTranslation(const Vec3& translation_in)
		{
			translation = translation_in;
		}
		Output operator()(const Vertex& in) const
		{
			//transform to world space
			Vec3 pos = in.pos * rotation + translation;
			//wave function
			pos.y += amplitude * std::sin(time * freqScroll + pos.x * freqWave);
			return { pos, in.t };
		}
		void SetTime(float t)
		{
			time = t;
		}
	private:
		Mat3 rotation;
		Vec3 translation;
		float amplitude =0.05f;
		float time = 0.0f;
		float freqScroll =5.0f;
		float freqWave = 10.0f;
		

	};
	// invoked for each pixel of a triangle
	// takes an input of attributes that are the
	// result of interpolating vertex attributes
	// and outputs a color
	class PixelShader
	{
	public:
		template<class I>
		Color operator()(const I& in) const
		{
			return pTex->GetPixel(
				(unsigned int)std::min(in.t.x * tex_width + 0.5f, tex_xclamp),
				(unsigned int)std::min(in.t.y * tex_height + 0.5f, tex_yclamp)
			);
		}
		void BindTexture(const std::wstring& filename)
		{
			pTex = std::make_unique<Surface>(Surface::FromFile(filename));
			tex_width = float(pTex->GetWidth());
			tex_height = float(pTex->GetHeight());
			tex_xclamp = tex_width - 1.0f;
			tex_yclamp = tex_height - 1.0f;
		}
	private:
		std::unique_ptr<Surface> pTex;
		float tex_width;
		float tex_height;
		float tex_xclamp;
		float tex_yclamp;
	};
public:
	VertexShader vs;
	PixelShader ps;

};