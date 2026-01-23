#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Data/Engine/Configs/Scene/Objects/Material/MaterialConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Transform/UvTransformConfig.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

#include <string>

struct BaseModelConfig {
	MaterialConfig	  materialConfig; // マテリアル設定
	Transform2DConfig uvTransConfig;  // UV 2D 変換
	int				  blendMode = 5;  // 初期値は通常ブレンド
	std::string		  modelName;	  // モデル名/パス
	Guid			  textureGuid{};  // テクスチャ GUID

	std::optional<std::string> legacyTextureName;
};

inline void to_json(nlohmann::json& j, const BaseModelConfig& c) {
	j = nlohmann::json{
		{"materialConfig", c.materialConfig},
		{"uvTransConfig", c.uvTransConfig},
		{"blendMode", c.blendMode},
		{"modelName", c.modelName},
		{"textureGuid", c.textureGuid}, // 保存は GUID のみ
	};
	// legacyTextureName は保存しない
}

inline void from_json(const nlohmann::json& j, BaseModelConfig& c) {
	j.at("materialConfig").get_to(c.materialConfig);
	j.at("uvTransConfig").get_to(c.uvTransConfig);
	j.at("blendMode").get_to(c.blendMode);
	j.at("modelName").get_to(c.modelName);

	if(auto it = j.find("textureGuid"); it != j.end() && !it->is_null()) {
		c.textureGuid = it->get<Guid>();
	} else {
		c.textureGuid = Guid::Empty();
	}

	if(auto it2 = j.find("textureName"); it2 != j.end() && it2->is_string()) {
		c.legacyTextureName = it2->get<std::string>();
	} else {
		c.legacyTextureName.reset();
	}
}