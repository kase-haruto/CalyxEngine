#pragma once

#include <externals/nlohmann/json.hpp>

namespace CalyxMath {
	struct Vector2{
		float x;
		float y;

		Vector2() = default;
		Vector2(float vx, float vy);
		Vector2(const Vector2& v);

		float          Length()const;
		static Vector2 Zero();
		static Vector2 Lerp(const Vector2& v1, const Vector2& v2, float t);
		float          LengthSquared() const;
		Vector2 Normalize();

		//--------- operator -----------------------------------------------------
		Vector2 operator+(const Vector2& v) const;
		Vector2 operator+=(const Vector2& v) const;
		Vector2 operator+(const float v)const;
		Vector2 operator-(const Vector2& v) const;
		Vector2 operator*(const Vector2& v)const;
		Vector2 operator*(const float v)const;
	};

	//--------- serializer ---------------------------------------------------
	inline void to_json(nlohmann::json& j, const Vector2& v) {
		j = nlohmann::json{ {"x", v.x}, {"y", v.y} };
	}

	inline void from_json(const nlohmann::json& j,Vector2& v) {
		j.at("x").get_to(v.x);
		j.at("y").get_to(v.y);
	}
}
