#pragma once

/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector2.h>

/**
 * @brief Transform2DConfigに関するデータを保持する構造体です。
 */
struct Transform2DConfig final {
    //========================= variable =========================
	CalyxEngine::Vector2 scale {1.0f,1.0f};		//<scale
	float rotation;					//<rotate
	CalyxEngine::Vector2 translation;			//<translate
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Transform2DConfig,
								   scale,
								   rotation,
								   rotation)
