#pragma once
/*=============================================================================
 *	include space
 *============================================================================*/

#include <externals/nlohmann/json.hpp>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>


namespace CalyxEngine {
	
	/*-----------------------------------------------------------------------------------------
	 *	Color
	 *	- RGBAカラーを表すクラス
	 *	- 色の表現と操作を提供
	 *	- 内部的には4次元ベクトル（R,G,B,A）で色を管理
	 *---------------------------------------------------------------------------------------*/
	struct Color {
		//===================================================================*/
		//			public method
		//===================================================================*/
		float r, g, b, a;

		constexpr Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
		constexpr Color(float _r, float _g, float _b, float _a = 1.0f) : r(_r), g(_g), b(_b), a(_a) {}
		constexpr Color(const ::CalyxEngine::Vector4& rgba) : r(rgba.x), g(rgba.y), b(rgba.z), a(rgba.w) {}
		constexpr Color(const ::CalyxEngine::Vector3& rgb, float _a = 1.0f) : r(rgb.x), g(rgb.y), b(rgb.z), a(_a) {}

		// combert
		::CalyxEngine::Vector4 ToVector4() const;
		::CalyxEngine::Vector3 ToVector3() const;

		// operators
		Color  operator*(float scalar) const;
		Color  operator+(const Color& other) const;
		Color& operator*=(float scalar);
		Color& operator+=(const Color& other);

		// preset
		static const Color Red;
		static const Color Green;
		static const Color Blue;
		static const Color White;
		static const Color Black;
	};

	//--------- serializer ---------------------------------------------------
	inline void to_json(nlohmann::json& j, const Color& c) {
		j = nlohmann::json{{"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a}};
	}
	inline void from_json(const nlohmann::json& j, Color& c) {
		j.at("r").get_to(c.r);
		j.at("g").get_to(c.g);
		j.at("b").get_to(c.b);
		j.at("a").get_to(c.a);
	}
}

