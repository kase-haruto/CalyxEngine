#pragma once

/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Data/Engine/Configs/Scene/Objects/Transform/WorldTransformConfig.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

#include <string>
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