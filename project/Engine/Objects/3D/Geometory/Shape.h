#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Quaternion.h>

#include <array>

struct OBB{
	CalyxEngine::Vector3 size;
	CalyxEngine::Quaternion rotate;
	CalyxEngine::Vector3 center;

	// 8頂点を返す関数
	std::array<CalyxEngine::Vector3, 8> GetVertices() const;

	void Draw();
};

struct Sphere{
	CalyxEngine::Vector3 center;
	float radius;

	void Draw(int subdivision = 8, CalyxEngine::Vector4 color = {1.0f,0.0f,0.0f,1.0f});
};