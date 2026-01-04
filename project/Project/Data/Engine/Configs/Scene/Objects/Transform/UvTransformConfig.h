#pragma once

/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector2.h>

struct Transform2DConfig final {
    //========================= variable =========================
	Vector2 scale {1.0f,1.0f};		//<scale
	float rotation;					//<rotate
	Vector2 translation;			//<translate
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Transform2DConfig,
								   scale,
								   rotation,
								   rotation)
