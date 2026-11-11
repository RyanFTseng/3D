#pragma once


#include "Pipeline.h"
#include "DefaultVertexShader.h"
#include "DefaultGeometryShader.h"

// solid color attribute taken from table in gs and not interpolated
//input pos
//output pos and color
class GeometryFlatEffect
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
	};
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
			Vec3 pos = in.pos * rotation + translation;
			pos.y += amplitude * std::sin(time * freqScroll + pos.x * freqWave);
			return{ pos};
		}
		void SetTime(float t)
		{
			time = t;
		}
	private:
		Mat3 rotation;
		Vec3 translation;
		float time = 0.0f;
		float freqWave = 10.0f;
		float freqScroll = 5.0f;
		float amplitude = 0.05f;
	};

	class GeometryShader
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
		//generate shader output
		Triangle<Output> operator()(const VertexShader::Output& in0, const VertexShader::Output& in1, const VertexShader::Output& in2, size_t triangle_index) const
		{
			//calc face normal with vertex cross product
			const auto n = ((in1.pos - in0.pos) % (in2.pos - in0.pos)).GetNormalized();
			//calc light intensity
			const auto d = diffuse * (std::max)(0.0f, -n * dir);
			//add difuse + ambient lighting, filter by material color, saturate, and scale
			const auto c = Color(color.GetHadamard(d + ambient).Saturate() * 255.0f);
			//return triangle
			return{ {in0.pos, c }, {in1.pos, c } , {in2.pos, c } };

		}
		void SetDiffuseLight(const Vec3& c)
		{
			diffuse = { c.x, c.y, c.z };
		}
		void SetAmbientLight(const Vec3& c)
		{
			ambient = { c.x, c.y, c.z };
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