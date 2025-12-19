#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Quaternion.h>

#include <array>

struct OBB{
	CalyxMath::Vector3 size;
	CalyxMath::Quaternion rotate;
	CalyxMath::Vector3 center;

	// 8頂点を返す関数
	std::array<CalyxMath::Vector3, 8> GetVertices() const;

	void Draw();
};

struct Sphere{
	CalyxMath::Vector3 center;
	float radius;

	void Draw(int subdivision = 8, CalyxMath::Vector4 color = {1.0f,0.0f,0.0f,1.0f});
};