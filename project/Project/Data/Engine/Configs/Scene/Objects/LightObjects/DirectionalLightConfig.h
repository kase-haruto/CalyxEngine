#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>
#include <string>
#include <externals/nlohmann/json.hpp>

struct DirectionalLightConfig : public SceneObjectConfig{
	Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};		//< 光の色
	Vector3 direction = {0.0f, -1.0f, 0.0f};		//< 光の方向
	float intensity = 0.25f;						//< 光の強度
};

inline void to_json(nlohmann::json& j, const DirectionalLightConfig& c){
	j = nlohmann::json {
		{"guid",        c.guid},
		{"parentGuid",  c.parentGuid},
		{"objectType",  c.objectType},
		{"name",        c.name},
		{"transform",   c.transform},
		{"color",       c.color},
		{"direction",   c.direction},
		{"intensity",   c.intensity}
	};
}

inline void from_json(const nlohmann::json& j, DirectionalLightConfig& c){
	j.at("guid").get_to(c.guid);
	j.at("parentGuid").get_to(c.parentGuid);
	j.at("objectType").get_to(c.objectType);
	j.at("name").get_to(c.name);
	j.at("transform").get_to(c.transform);
	j.at("color").get_to(c.color);
	j.at("direction").get_to(c.direction);
	j.at("intensity").get_to(c.intensity);
}
