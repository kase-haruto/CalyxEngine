#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Objects/Transform/Transform.h>

/*-----------------------------------------------------------------------------------------
 * TransformSnapshot
 * - Editor CommandがWorldTransformの変更前後を保持するデータ構造
 * - Scale、Rotation、Translationの取得、比較、復元を担当する
 * - Transform本体の所有権は持たない
 *---------------------------------------------------------------------------------------*/
struct TransformSnapshot {
public:
	static TransformSnapshot FromTransform(const WorldTransform* tf);
	void ApplyToTransform(WorldTransform* tf) const;

	bool Equals(const TransformSnapshot& other, float epsilon = 1e-5f) const;

	CalyxEngine::Vector3 scale;
	CalyxEngine::Quaternion rotate;
	CalyxEngine::Vector3 translate;
};
