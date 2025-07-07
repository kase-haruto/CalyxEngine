#include "Vector2.h"



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
