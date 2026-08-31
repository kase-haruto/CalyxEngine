#pragma once

/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Data/Engine/Configs/Scene/Objects/Transform/WorldTransformConfig.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

#include <string>

/*-----------------------------------------------------------------------------------------
 * SceneObjectConfig
 * - 全SceneObjectに共通するシリアライズ設定を保持するデータ構造
 * - GUID、親子関係、型、名前、TransformをScene保存形式として管理する
 *---------------------------------------------------------------------------------------*/
/**
 * @brief SceneObjectConfigに関するデータを保持する構造体です。
 */
struct SceneObjectConfig{
	Guid guid {};						//< ID
	Guid parentGuid {};					//< 親ID
	int objectType = 0;					//< オブジェクトの種類
	std::string name {};				//< オブジェクト名
	WorldTransformConfig transform {};	//< ワールドトランスフォーム
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SceneObjectConfig,
								   guid,
								   parentGuid,
								   objectType,
								   name, transform)
