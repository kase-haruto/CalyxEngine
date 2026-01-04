#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */

#include <externals/nlohmann/json.hpp>

struct ColliderConfig {
	//========================= variable =========================
	bool isCollisionEnabled = true;		//< コリジョン有効フラグ
	bool isDraw = true;					//< 描画有効フラグ
	int colliderType = 0;				//< コリジョンの種類
	int targetType = 0;					//< 衝突相手の種類
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ColliderConfig,
								   isCollisionEnabled,
								   isDraw,
								   colliderType,
								   targetType)