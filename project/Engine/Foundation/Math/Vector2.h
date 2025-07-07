#pragma once

#include <externals/nlohmann/json.hpp>

struct Vector2{
	float x;
	float y;



	//--------- operator -----------------------------------------------------
	Vector2 operator+(const Vector2& v) const;
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