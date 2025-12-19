#pragma once

#include <Engine/Foundation/Math/Vector3.h>

struct Ray {
	CalyxMath::Vector3 origin;		// レイの開始点（カメラ位置など）
	CalyxMath::Vector3 direction;	// 正規化済みの方向ベクトル

	Ray() = default;
	Ray(const CalyxMath::Vector3& o, const CalyxMath::Vector3& d) : origin(o), direction(d){}
};

struct RaycastHit{
	float distance = 0.0f;	// ヒットした距離
	CalyxMath::Vector3 point;			// ヒットした点
	CalyxMath::Vector3 normal;			// ヒットした面の法線ベクトル
	void* hitObject = nullptr;
};