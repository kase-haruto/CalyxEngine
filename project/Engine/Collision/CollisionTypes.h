#pragma once

#include <cstdint>

// Collision Layer を参照するための軽量な数値 ID。
// Collider が文字列を直接保持すると、Layer のリネーム時に全シーンとPrefabの更新が必要になる。
// そのためColliderにはIDだけを保存し、表示名との対応はCollisionLayerSettingsへ集約する。
// また、ゲーム固有の分類をenumとして定義しないことで、エンジンを別プロジェクトでも再利用できる。
using CollisionLayerId = uint8_t;

// ID 0は、Layer設定のない既存データや新規Colliderが参照する共通のLayerとして予約する。
constexpr CollisionLayerId kDefaultCollisionLayerId = 0;

// Matrixの1行をuint32_tのビット列で保持するため、Layer数は32に制限する。
// CollisionLayerId自体には余裕を持たせているが、有効範囲は必ずこの定数で判定する。
constexpr uint32_t kMaxCollisionLayerCount = 32;

inline uint32_t ToCollisionLayerBit(CollisionLayerId layerId) {
	// 32以上のシフトは未定義動作になるため、範囲外IDではbitを生成しない。
	return layerId < kMaxCollisionLayerCount ? (1u << layerId) : 0u;
}
