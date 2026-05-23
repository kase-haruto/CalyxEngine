#pragma once

#include <Data/Engine/Configs/Scene/Objects/Collider/ColliderConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Model/BaseModelConfig.h>
#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>
#include <Engine/Foundation/Math/Vector4.h>

#include <externals/nlohmann/json.hpp>

/*-----------------------------------------------------------------------------------------
 * BaseGameObjectConfig
 * - ゲームオブジェクト設定構造体
 * - SceneObjectConfig・Collider・Modelの設定を統合した基底設定
 *---------------------------------------------------------------------------------------*/
struct BaseGameObjectConfig
	: public SceneObjectConfig {
	ColliderConfig	colliderConfig;
	BaseModelConfig modelConfig;
	bool outlineEnabled = true;
	float outlineThickness = 0.035f;
	CalyxEngine::Vector4 outlineColor = {0.02f, 0.02f, 0.025f, 1.0f};
};

inline void to_json(nlohmann::json& j, const BaseGameObjectConfig& c) {
	j = nlohmann::json{
		{"guid", c.guid},
		{"parentGuid", c.parentGuid},
		{"objectType", c.objectType},
		{"name", c.name},
		{"transform", c.transform},
		{"colliderConfig", c.colliderConfig},
		{"modelConfig", c.modelConfig},
		{"outlineEnabled", c.outlineEnabled},
		{"outlineThickness", c.outlineThickness},
		{"outlineColor", c.outlineColor}};
}

inline void from_json(const nlohmann::json& j, BaseGameObjectConfig& c) {
	if(j.contains("guid")) j.at("guid").get_to(c.guid);
	if(j.contains("parentGuid")) j.at("parentGuid").get_to(c.parentGuid);
	if(j.contains("objectType")) j.at("objectType").get_to(c.objectType);
	if(j.contains("name")) j.at("name").get_to(c.name);
	if(j.contains("transform")) j.at("transform").get_to(c.transform);
	if(j.contains("colliderConfig")) j.at("colliderConfig").get_to(c.colliderConfig);
	if(j.contains("modelConfig")) j.at("modelConfig").get_to(c.modelConfig);
	if(j.contains("outlineEnabled")) j.at("outlineEnabled").get_to(c.outlineEnabled);
	if(j.contains("outlineThickness")) j.at("outlineThickness").get_to(c.outlineThickness);
	if(j.contains("outlineColor")) j.at("outlineColor").get_to(c.outlineColor);
}
