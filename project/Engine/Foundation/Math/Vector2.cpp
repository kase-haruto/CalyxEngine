#include "Vector2.h"



float Vector2::Length() const{
	return sqrtf(x * x + y * y);
}

//--------- operator -----------------------------------------------------
Vector2 Vector2::operator+(const Vector2& v) const{
	return {x + v.x, y + v.y};
}

Vector2 Vector2::operator+(const float v)const {
	return {x + v, y + v};
}

Vector2 Vector2::operator-(const Vector2& v) const{
	return {x - v.x, y - v.y};
}

Vector2 Vector2::operator*(const Vector2& v) const{
	return {x * v.x,y * v.y};
}

Vector2 Vector2::operator*(const float v) const{
	return {x * v,y * v};
}
