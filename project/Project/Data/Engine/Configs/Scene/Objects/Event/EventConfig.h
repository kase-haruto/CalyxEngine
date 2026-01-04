#pragma once

#include <Data/Engine/Configs/Scene/Objects/Collider/ColliderConfig.h>
#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>

#include <externals/nlohmann/json.hpp>

struct EventConfig
	: public SceneObjectConfig {
	ColliderConfig colliderConfig;
};

inline void to_json(nlohmann::json& j, const EventConfig& c) {
	// まずは基底クラスの情報を書き込む
	to_json(j, static_cast<const SceneObjectConfig&>(c));
	
	// 追加フィールドを書き足す
	j["colliderConfig"] = c.colliderConfig;
}

inline void from_json(const nlohmann::json& j, EventConfig& c) {
	// まずは基底クラスを復元
	from_json(j, static_cast<SceneObjectConfig&>(c));

	// 追加フィールドを復元
	if (j.contains("colliderConfig"))
		j.at("colliderConfig").get_to(c.colliderConfig);
}