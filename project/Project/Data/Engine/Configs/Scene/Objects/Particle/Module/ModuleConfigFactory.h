#pragma once

#include <memory>
#include "ModuleConfig.h"
#include <externals/nlohmann/json.hpp>

namespace ModuleConfigFactory{

	inline std::unique_ptr<BaseModuleConfig> FromJson(const nlohmann::json& j){
		if (!j.contains("name") || !j.at("name").is_string()){
			return nullptr;
		}

		std::string name = j.at("name").get<std::string>();
		std::unique_ptr<BaseModuleConfig> modConfig;

		if (name == "GravityModule"){
			modConfig = std::make_unique<GravityModuleConfig>();
		} else if (name == "SizeOverLifetimeModule"){
			modConfig = std::make_unique<SizeOverLifetimeConfig>();
		} else if (name == "TextureSheetAnimationModule"){
			modConfig = std::make_unique<TextureSheetAnimationConfig>();
		}
		// 今後モジュールを増やす場合はここに追加

		// デフォルト: 該当なし
		if (!modConfig){
			return nullptr;
		}

		// JSONを反映
		modConfig->FromJson(j);
		return modConfig;
	}

}
