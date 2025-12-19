#pragma once

/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Quaternion.h>

struct WorldTransformConfig final {
    //========================= variable =========================
	CxMath::Vector3 translation = { 0.0f, 0.0f, 0.0f };			//< 位置
	CxMath::Quaternion rotation = CxMath::Quaternion::MakeIdentity();	//< 回転
	CxMath::Vector3 scale = { 1.0f, 1.0f, 1.0f };				//< スケール
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WorldTransformConfig, translation,rotation,scale)