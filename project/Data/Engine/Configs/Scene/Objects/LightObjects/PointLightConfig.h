#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>

#include <string>
#include <externals/nlohmann/json.hpp>

struct PointLightConfig : public SceneObjectConfig{
	//========================= variable =========================
	CalyxMath::Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};	//< 光の色
	CalyxMath::Vector3 position = {0.0f, 0.0f, 0.0f};		//< ワールド位置
	float intensity = 0.25f;					//< 光の強度
	float radius = 20.0f;						//< 最大距離
	float decay = 1.0f;							//< 減衰率
};

// JSON定義（手動で拡張）
inline void to_json(nlohmann::json& j, const PointLightConfig& c){
	j = nlohmann::json {
		{"guid",        c.guid},
		{"parentGuid",  c.parentGuid},
		{"objectType",  c.objectType},
		{"name",        c.name},
		{"transform",   c.transform},
		{"color",       c.color},
		{"position",    c.position},
		{"intensity",   c.intensity},
		{"radius",      c.radius},
		{"decay",       c.decay}
	};
}

inline void from_json(const nlohmann::json& j, PointLightConfig& c){
	j.at("guid").get_to(c.guid);
	j.at("parentGuid").get_to(c.parentGuid);
	j.at("objectType").get_to(c.objectType);
	j.at("name").get_to(c.name);
	j.at("transform").get_to(c.transform);
	j.at("color").get_to(c.color);
	j.at("position").get_to(c.position);
	j.at("intensity").get_to(c.intensity);
	j.at("radius").get_to(c.radius);
	j.at("decay").get_to(c.decay);
}
