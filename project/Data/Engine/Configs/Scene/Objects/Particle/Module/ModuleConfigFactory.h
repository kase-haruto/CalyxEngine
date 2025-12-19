#pragma once

#include "ModuleConfig.h"
#include <externals/nlohmann/json.hpp>
#include <memory>

namespace CalyxEffect {
	namespace ModuleConfigFactory {

		inline std::unique_ptr<CalyxEffect::BaseModuleConfig> FromJson(const nlohmann::json& j) {
			if(!j.contains("name") || !j.at("name").is_string()) {
				return nullptr;
			}

			std::string						  name = j.at("name").get<std::string>();
			std::unique_ptr<CalyxEffect::BaseModuleConfig> modConfig;

			if(name == "GravityModule") {
				modConfig = std::make_unique<GravityModuleConfig>();
			} else if(name == "SizeOverLifetimeModule") {
				modConfig = std::make_unique<SizeOverLifetimeConfig>();
			} else if(name == "TextureSheetAnimationModule") {
				modConfig = std::make_unique<TextureSheetAnimationConfig>();
			} else if(name == "OverLifetimeModule") {
				modConfig = std::make_unique<OverLifetimeModuleConfig>();
			}

			if(!modConfig) {
				return nullptr;
			}

			modConfig->FromJson(j);
			return modConfig;
		}

	} // namespace ModuleConfigFactory

} // namespace CalyxEffect
