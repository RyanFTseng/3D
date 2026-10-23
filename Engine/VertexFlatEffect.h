#pragma once

#include "Pipeline.h"
#include "DefaultVertexShader.h"
#include "DefaultGeometryShader.h"

// solid color attribute taken from table in gs and not interpolated
//input pos
//output pos and color
class VertexFlatEffect
{
public:
	// the vertex type that will be input into the pipeline
	//contains n as vertex normal data that is defined when cube is called in the scene
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
		Vec3 n;
	};
	// default vs rotates and translates vertices
	// does not touch attributes
	// gs colors vertices using their index from a table
	// every two triangles are colored from the same entry
	class VertexShader
	{
	public:
		class Output
		{
		public:
			Output() = default;
			Output(const Vec3& pos)
				:
				pos(pos)
			{}
			Output(const Vec3& pos, const Output& src)
				:
				color(src.color),
				pos(pos)
			{}
			Output(const Vec3& pos, const Color& color)
				:
				color(color),
				pos(pos)
			{}
			Output& operator+=(const Output& rhs)
			{
				pos += rhs.pos;
				return *this;
			}
			Output operator+(const Output& rhs) const
			{
				return Output(*this) += rhs;
			}
			Output& operator-=(const Output& rhs)
			{
				pos -= rhs.pos;
				return *this;
			}
			Output operator-(const Output& rhs) const
			{
				return Output(*this) -= rhs;
			}
			Output& operator*=(float rhs)
			{
				pos *= rhs;
				return *this;
			}
			Output operator*(float rhs) const
			{
				return Output(*this) *= rhs;
			}
			Output& operator/=(float rhs)
			{
				pos /= rhs;
				return *this;
			}
			Output operator/(float rhs) const
			{
				return Output(*this) /= rhs;
			}
		public:
			Vec3 pos;
			Color color;
		};
	public:
		void BindRotation(const Mat3& rotation_in)
		{
			rotation = rotation_in;
		}
		void BindTranslation(const Vec3& translation_in)
		{
			translation = translation_in;
		}
		Output operator()(const Vertex &v) const
		{
			
			// calculate intensity based on angle of incidence
			const auto d = diffuse * std::max(0.0f, -(v.n * rotation) * dir);
			// add diffuse+ambient, filter by material color, saturate and scale
			const auto c = color.GetHadamard(d + ambient).Saturate() * 255.0f;
			return{ v.pos * rotation + translation,Color(c) };
		};
		void SetDiffuseLight(const Vec3& c)
		{
			diffuse = { c.x,c.y,c.z };
		}
		void SetAmbientLight(const Vec3& c)
		{
			ambient = { c.x,c.y,c.z };
		}
		void SetLightDirection(const Vec3& dl)
		{
			assert(dl.LenSq() >= 0.001f);
			dir = dl.GetNormalized();
		}
		void SetMaterialColor(Color c)
		{
			color = Vec3(c);
		}
		
	private:
		std::vector<Color> triangle_colors;
		Mat3 rotation;
		Vec3 translation;
		Vec3 dir = { 0.0f,0.0f,1.0f };
		Vec3 diffuse = { 1.0f,1.0f,1.0f };
		Vec3 ambient = { 0.1f,0.1f,0.1f };
		Vec3 color = { 0.8f,0.85f,1.0f };
	};
	typedef DefaultGeometryShader<VertexShader::Output> GeometryShader;
	// invoked for each pixel of a triangle
	// takes an input of attributes that are the
	// result of interpolating vertex attributes
	// and outputs a color
	class PixelShader
	{
	public:
		template<class Input>
		Color operator()(const Input& in) const
		{
			return in.color;
		}

	};
public:
	VertexShader vs;
	GeometryShader gs;
	PixelShader ps;
};