#pragma once

#include <Data/Engine/Configs/Scene/Objects/Collider/ColliderConfig.h>
#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>

#include <externals/nlohmann/json.hpp>

struct EventConfig
	: public SceneObjectConfig {
	ColliderConfig colliderConfig;
};

inline void to_json(nlohmann::json& j,const EventConfig& c) {
	j = nlohmann::json{
			{"guid",c.guid},
			{"parentGuid",c.parentGuid},
			{"objectType",c.objectType},
			{"name",c.name},
			{"transform",c.transform},
			{"colliderConfig",c.colliderConfig},
		};
}

inline void from_json(const nlohmann::json& j,EventConfig& c) {
	j.at("guid").get_to(c.guid);
	j.at("parentGuid").get_to(c.parentGuid);
	j.at("objectType").get_to(c.objectType);
	j.at("name").get_to(c.name);
	j.at("transform").get_to(c.transform);
	j.at("colliderConfig").get_to(c.colliderConfig);
}