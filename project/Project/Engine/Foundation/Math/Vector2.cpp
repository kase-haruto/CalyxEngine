#include "Vector2.h"


Vector2::Vector2(float vx, float vy) : x(vx), y(vy){}

Vector2::Vector2(const Vector2& v){
	x = v.x; y = v.y;
}

float Vector2::Length() const{
	return sqrtf(x * x + y * y);
}

//--------- operator -----------------------------------------------------
Vector2 Vector2::operator+(const Vector2& v) const{
	return {x + v.x, y + v.y};
}

Vector2 Vector2::operator+(const float v)const{
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
