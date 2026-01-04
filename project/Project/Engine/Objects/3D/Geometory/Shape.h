#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Quaternion.h>

#include <array>

struct OBB{
	Vector3 size;
	Quaternion rotate;
	Vector3 center;

	// 8頂点を返す関数
	std::array<Vector3, 8> GetVertices() const;

	void Draw();
};

struct Sphere{
	Vector3 center;
	float radius;

	void Draw(int subdivision = 8, Vector4 color = {1.0f,0.0f,0.0f,1.0f});
};