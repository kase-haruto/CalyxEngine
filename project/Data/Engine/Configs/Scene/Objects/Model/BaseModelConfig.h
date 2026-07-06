#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Data/Engine/Configs/Scene/Objects/Material/MaterialConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Transform/UvTransformConfig.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

#include <optional>
#include <string>

struct BaseModelConfig {
	Guid			  materialGuid{}; // マテリアル GUID
	Transform2DConfig uvTransConfig;  // UV 2D 変換
	int				  blendMode = 5;  // 初期値は通常ブレンド
	std::string		  modelName;	  // モデル名/パス
	Guid			  textureGuid{};  // テクスチャ GUID

	std::optional<CalyxEngine::Vector4> colorOverride;
	std::optional<std::string> legacyTextureName;
};

inline void to_json(nlohmann::json& j, const BaseModelConfig& c) {
	j = nlohmann::json{
		{"materialGuid", c.materialGuid},
		{"uvTransConfig", c.uvTransConfig},
		{"blendMode", c.blendMode},
		{"modelName", c.modelName},
		{"textureGuid", c.textureGuid},
	};
	if(c.colorOverride) {
		j["colorOverride"] = *c.colorOverride;
	}
	// legacyTextureName は保存しない
}

inline void from_json(const nlohmann::json& j, BaseModelConfig& c) {
	if(auto it = j.find("materialGuid"); it != j.end() && !it->is_null()) {
		c.materialGuid = it->get<Guid>();
	} else {
		c.materialGuid = Guid::Empty();
	}
	j.at("uvTransConfig").get_to(c.uvTransConfig);
	j.at("blendMode").get_to(c.blendMode);
	j.at("modelName").get_to(c.modelName);

	if(auto it = j.find("textureGuid"); it != j.end() && !it->is_null()) {
		c.textureGuid = it->get<Guid>();
	} else {
		c.textureGuid = Guid::Empty();
	}

	if(auto it = j.find("colorOverride"); it != j.end() && !it->is_null()) {
		c.colorOverride = it->get<CalyxEngine::Vector4>();
	} else {
		c.colorOverride.reset();
	}

	if(auto it2 = j.find("textureName"); it2 != j.end() && it2->is_string()) {
		c.legacyTextureName = it2->get<std::string>();
	} else {
		c.legacyTextureName.reset();
	}
}
