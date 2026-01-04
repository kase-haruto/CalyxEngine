#pragma once

#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/EmitterConfig.h>

struct ParticleSystemObjectConfig
	: public SceneObjectConfig, public EmitterConfig{};

// JSON変換
inline void to_json(nlohmann::json& j, const ParticleSystemObjectConfig& c){
	// ベース構造体をまず個別に展開
	nlohmann::json emitterJson = c.EmitterConfig::ToJson();  // パーティクル設定
	j = emitterJson;

	j["guid"] = c.guid;
	j["parentGuid"] = c.parentGuid;
	j["objectType"] = c.objectType;
	j["name"] = c.name;
	j["transform"] = c.transform;
}

inline void from_json(const nlohmann::json& j, ParticleSystemObjectConfig& c){
	// 安全にキーを取得、無ければデフォルト値
	c.guid = j.value("guid", Guid {});
	c.parentGuid = j.value("parentGuid", Guid {});
	c.name = j.value("name", std::string {});

	if (!j.is_null()){
		c.EmitterConfig::FromJson(j);
	}
}
