#include "CollisionMatrix.h"

CollisionMatrix::CollisionMatrix() = default;

bool CollisionMatrix::CanCollide(CollisionLayerId layerA, CollisionLayerId layerB) const {
	// 不正 ID でシフトや配列範囲外アクセスを起こさないため、安全側の false を返す。
	if(!IsLayerIdInRange(layerA) || !IsLayerIdInRange(layerB)) {
		return false;
	}

	// 行Aから列Bに対応する1bitだけを取り出す。
	// Settings側で存在確認を行うが、Matrix単体でも範囲チェックを行い防御を二重化している。
	return (rows_[layerA] & ToCollisionLayerBit(layerB)) != 0u;
}

void CollisionMatrix::SetCanCollide(CollisionLayerId layerA, CollisionLayerId layerB, bool canCollide) {
	// 無効な ID に対する編集は状態を変更せず無視する。
	if(!IsLayerIdInRange(layerA) || !IsLayerIdInRange(layerB)) {
		return;
	}

	const uint32_t bitA = ToCollisionLayerBit(layerA);
	const uint32_t bitB = ToCollisionLayerBit(layerB);
	if(canCollide) {
		// A行のB列とB行のA列を同時にONにする。
		rows_[layerA] |= bitB;
		rows_[layerB] |= bitA;
	} else {
		// 対象bitだけを反転マスクで落とし、同じ行の他設定は維持する。
		rows_[layerA] &= ~bitB;
		rows_[layerB] &= ~bitA;
	}
	// 衝突は双方向の関係なので、片側の変更時に反対側も更新して対称性を保証する。
}

void CollisionMatrix::ClearLayer(CollisionLayerId layerId) {
	// 削除要求に不正IDが混ざっても、既存Matrixを壊さず終了する。
	if(!IsLayerIdInRange(layerId)) {
		return;
	}

	// SetCanCollideを経由することで、対象行だけでなく全行にある対象列も確実に消去する。
	for(CollisionLayerId other = 0; other < kMaxCollisionLayerCount; ++other) {
		SetCanCollide(layerId, other, false);
	}
}

bool CollisionMatrix::IsLayerIdInRange(CollisionLayerId layerId) {
	// uint8_tは32以上も表現できるため、型だけに依存せず明示的に最大数を確認する。
	return layerId < kMaxCollisionLayerCount;
}
