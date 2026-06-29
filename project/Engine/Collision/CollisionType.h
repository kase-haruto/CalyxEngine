#pragma once

// c++
#include <cstdint>

namespace CalyxEngine{

	using CollisionLayerID = uint16_t; //< CollisionLayerID は 16bit のビットマスクで表現する

	CollisionLayerID kDefaultCollisionLayerID = 0x0001; //< デフォルトの衝突レイヤーID（ビットマスク）
	uint32_t kMaxCollisionLayerCount = 32; //< 衝突レイヤーの最大数（32bitのビットマスクで表現するため）

	uint16_t ToCollisionBits(CollisionLayerID layerID) {
		// layerID をビットマスクに変換
		// 例: layerID = 0 → 0b000000000000
		return 1u << layerID;
	}

}