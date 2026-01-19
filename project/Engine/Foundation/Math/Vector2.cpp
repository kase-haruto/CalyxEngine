#include "Vector2.h"

namespace CalyxMath {
	Vector2::Vector2(float vx, float vy) : x(vx), y(vy) {}

	Vector2::Vector2(const Vector2& v) {
		x = v.x;
		y = v.y;
	}

	float Vector2::Length() const {
		return sqrtf(x * x + y * y);
	}

	Vector2 Vector2::Zero() {
		return Vector2(0.0f, 0.0f);
	}

	Vector2 Vector2::Lerp(const Vector2& v1, const Vector2& v2, float t) {

		return Vector2(
			v1.x + t * (v2.x - v1.x),
			v1.y + t * (v2.y - v1.y));
	}

	float Vector2::LengthSquared() const {
		return x * x + y * y;
	}

	Vector2 Vector2::Normalize() {
		float length = Length();
		return Vector2(x / length, y / length);
	}

	//--------- operator -----------------------------------------------------
	Vector2 Vector2::operator+(const Vector2& v) const {
		return {x + v.x, y + v.y};
	}
	Vector2 Vector2::operator+=(const Vector2& v) const {
		return {x + v.x, y + v.y};
	}

	Vector2 Vector2::operator+(const float v) const {
		return {x + v, y + v};
	}

	Vector2 Vector2::operator-(const Vector2& v) const {
		return {x - v.x, y - v.y};
	}

	Vector2 Vector2::operator*(const Vector2& v) const {
		return {x * v.x, y * v.y};
	}

	Vector2 Vector2::operator*(const float v) const {
		return {x * v, y * v};
	}
} // namespace CalyxMath
