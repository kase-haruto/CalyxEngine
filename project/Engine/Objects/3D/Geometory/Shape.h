#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Quaternion.h>

#include <array>

struct OBB{
	CxMath::Vector3 size;
	CxMath::Quaternion rotate;
	CxMath::Vector3 center;

	// 8頂点を返す関数
	std::array<CxMath::Vector3, 8> GetVertices() const;

	void Draw();
};

struct Sphere{
	CxMath::Vector3 center;
	float radius;

	void Draw(int subdivision = 8, CxMath::Vector4 color = {1.0f,0.0f,0.0f,1.0f});
};