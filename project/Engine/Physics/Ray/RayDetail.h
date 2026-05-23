#pragma once

#include <Engine/Foundation/Math/Vector3.h>

struct Ray {
	CalyxEngine::Vector3 origin;		// レイの開始点（カメラ位置など）
	CalyxEngine::Vector3 direction;	// 正規化済みの方向ベクトル

	Ray() = default;
	Ray(const CalyxEngine::Vector3& o, const CalyxEngine::Vector3& d) : origin(o), direction(d){}
};

struct RaycastHit{
	float distance = 0.0f;	// ヒットした距離
	CalyxEngine::Vector3 point;			// ヒットした点
	CalyxEngine::Vector3 normal;			// ヒットした面の法線ベクトル
	void* hitObject = nullptr;
};