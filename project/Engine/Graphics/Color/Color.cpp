#include "Color.h"

using namespace CalyxEngine;
using namespace ::CalyxMath;

///////////////////////////////////////////////////////////////////////////////
//			変換
///////////////////////////////////////////////////////////////////////////////
Vector4 Color::ToVector4() const {
	return Vector4(r, g, b, a);
}

Vector3 Color::ToVector3() const {
	return Vector3(r, g, b);
}

///////////////////////////////////////////////////////////////////////////////
//			演算子
///////////////////////////////////////////////////////////////////////////////
Color Color::operator*(float scalar) const {
	return Color(r * scalar, g * scalar, b * scalar, a * scalar);
}

Color Color::operator+(const Color& other) const {
	return Color(r + other.r, g + other.g, b + other.b, a + other.a);
}

Color& Color::operator*=(float scalar) {
	r *= scalar;
	g *= scalar;
	b *= scalar;
	a *= scalar;
	return *this;
}

Color& Color::operator+=(const Color& other) {
	r += other.r;
	g += other.g;
	b += other.b;
	a += other.a;
	return *this;
}

///////////////////////////////////////////////////////////////////////////////
//			定数
///////////////////////////////////////////////////////////////////////////////
const Color Color::Red   (1.0f, 0.0f, 0.0f, 1.0f);
const Color Color::Green (0.0f, 1.0f, 0.0f, 1.0f);
const Color Color::Blue  (0.0f, 0.0f, 1.0f, 1.0f);
const Color Color::White (1.0f, 1.0f, 1.0f, 1.0f);
const Color Color::Black (0.0f, 0.0f, 0.0f, 1.0f);
