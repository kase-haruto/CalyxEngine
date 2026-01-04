#pragma once

#include <Data/Engine/Configs/Scene/Objects/Collider/ColliderConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Model/BaseModelConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Transform/WorldTransformConfig.h>
#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>

#include <Engine/Foundation/Math/Vector3.h>
#include <externals/nlohmann/json.hpp>

struct BaseGameObjectConfig
	: public SceneObjectConfig{
	ColliderConfig   colliderConfig;
	BaseModelConfig  modelConfig;
};

inline void to_json(nlohmann::json& j, const BaseGameObjectConfig& c){
	j = nlohmann::json {
		{"guid",         c.guid},
		{"parentGuid",   c.parentGuid},
		{"objectType",   c.objectType},
		{"name",         c.name},
		{"transform",    c.transform},
		{"colliderConfig", c.colliderConfig},
		{"modelConfig",    c.modelConfig}
	};
}

inline void from_json(const nlohmann::json& j, BaseGameObjectConfig& c){
	j.at("guid").get_to(c.guid);
	j.at("parentGuid").get_to(c.parentGuid);
	j.at("objectType").get_to(c.objectType);
	j.at("name").get_to(c.name);
	j.at("transform").get_to(c.transform);
	j.at("colliderConfig").get_to(c.colliderConfig);
	j.at("modelConfig").get_to(c.modelConfig);
}
