#pragma once

#include <Engine/Foundation/Math/Vector3.h>

/*-----------------------------------------------------------------------------------------
 * Ray
 * - Raycastに使用する半直線を表すデータ構造
 * - World空間の始点と正規化済み方向を保持する
 *---------------------------------------------------------------------------------------*/
struct Ray {
	CalyxEngine::Vector3 origin;		// レイの開始点（カメラ位置など）
	CalyxEngine::Vector3 direction;	// 正規化済みの方向ベクトル

	Ray() = default;
	Ray(const CalyxEngine::Vector3& o, const CalyxEngine::Vector3& d) : origin(o), direction(d){}
};

/*-----------------------------------------------------------------------------------------
 * RaycastHit
 * - Raycastで検出した交差情報を保持するデータ構造
 * - 距離、交点、法線とHit対象への非所有参照を管理する
 *---------------------------------------------------------------------------------------*/
struct RaycastHit{
	float distance = 0.0f;	// ヒットした距離
	CalyxEngine::Vector3 point;			// ヒットした点
	CalyxEngine::Vector3 normal;			// ヒットした面の法線ベクトル
	void* hitObject = nullptr;
};
