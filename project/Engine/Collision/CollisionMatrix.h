#pragma once

#include <Engine/Collision/CollisionTypes.h>

#include <array>
#include <cstdint>

/*-----------------------------------------------------------------------------------------
 * CollisionMatrix
 * - Layer ID の組み合わせごとに形状判定を実行するかを管理する。
 * - 判定結果は方向に依存しないため、常に A-B と B-A を同じ値に保つ。
 *---------------------------------------------------------------------------------------*/
class CollisionMatrix {
public:
	// 初期状態は全組み合わせfalse。実際に存在するLayerだけをSettings側が有効化する。
	CollisionMatrix();

	// 2つのLayer間で形状判定を実行してよいかを返す。
	bool CanCollide(CollisionLayerId layerA, CollisionLayerId layerB) const;

	// 指定セルと転置セルを同時に更新し、Matrixの対称性を維持する。
	void SetCanCollide(CollisionLayerId layerA, CollisionLayerId layerB, bool canCollide);

	// Layer削除時に、そのLayerに関係する行・列をまとめて無効化する。
	void ClearLayer(CollisionLayerId layerId);

private:
	static bool IsLayerIdInRange(CollisionLayerId layerId);

	// rows_[A]のbit Bが、Layer AとLayer Bの衝突可否を表す。
	// 各行を32bitのビット列として保持し、32x32のbool配列より小さく扱う。
	std::array<uint32_t, kMaxCollisionLayerCount> rows_{};
};
