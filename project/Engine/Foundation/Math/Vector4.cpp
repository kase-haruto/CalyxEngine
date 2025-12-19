#include "Vector4.h"
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector3.h>

namespace CalyxMath {

	Vector4::Vector4(const Vector3& v, float w)
		: x(v.x), y(v.y), z(v.z), w(w) {}

	Vector4 Vector4::TransformVector(const CalyxMath::Matrix4x4& m, const Vector4& v) {
		return Vector4{
			m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * v.w,
			m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * v.w,
			m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * v.w,
			m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3] * v.w,
		};
	}

	Vector4 Vector4::Transform(const Vector4& v, const CalyxMath::Matrix4x4& m) {
		return {
			v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + v.w * m.m[3][0],
			v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + v.w * m.m[3][1],
			v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + v.w * m.m[3][2],
			v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + v.w * m.m[3][3]};
	}
	Vector4 Vector4::Lerp(const Vector4& a, const Vector4& b, float t) noexcept {
		if(t <= 0.0f) return a;
		if(t >= 1.0f) return b;
		return LerpUnclamped(a, b, t);
	}
	inline Vector4 Vector4::LerpUnclamped(const Vector4& a, const Vector4& b, float t) noexcept {
		return Vector4{
			a.x + (b.x - a.x) * t,
			a.y + (b.y - a.y) * t,
			a.z + (b.z - a.z) * t,
			a.w + (b.w - a.w) * t};
	}

	bool Vector4::operator==(const Vector4& other) const {
		return x == other.x && y == other.y && z == other.z && w == other.w;
	}

	bool Vector4::operator!=(const Vector4& other) const {
		return !(*this == other);
	}

	Vector4 Vector4::operator*(const float& scalar) const {
		return Vector4(x * scalar, y * scalar, z * scalar, w * scalar);
	}

	Vector4 Vector4::operator*=(const float& scalar) {
		x *= scalar;
		y *= scalar;
		z *= scalar;
		w *= scalar;
		return *this;
	}

	Vector4 Vector4::operator*(const Vector4& other) const {
		return Vector4(x * other.x, y * other.y, z * other.z, w * other.w);
	}

	Vector4 Vector4::operator*=(const Vector4& other) {
		x *= other.x;
		y *= other.y;
		z *= other.z;
		w *= other.w;
		return *this;
	}

	Vector4 Vector4::operator/(const float& scalar) const {
		return Vector4(x / scalar, y / scalar, z / scalar, w / scalar);
	}

	Vector4 Vector4::operator/=(const float& scalar) {
		x /= scalar;
		y /= scalar;
		z /= scalar;
		w /= scalar;
		return *this;
	}

	Vector3 Vector4::xyz() const {
		return Vector3{x, y, z};
	}

} // namespace MyNamespace