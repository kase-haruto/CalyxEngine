#pragma once

#include "ModuleConfig.h"
#include <externals/nlohmann/json.hpp>
#include <memory>

namespace CalyxEngine {
	namespace ModuleConfigFactory {

		inline std::unique_ptr<CalyxEngine::BaseModuleConfig> FromJson(const nlohmann::json& j) {
			if(!j.contains("name") || !j.at("name").is_string()) {
				return nullptr;
			}

			std::string						  name = j.at("name").get<std::string>();
			std::unique_ptr<CalyxEngine::BaseModuleConfig> modConfig;

			if(name == "GravityModule") {
				modConfig = std::make_unique<GravityModuleConfig>();
			} else if(name == "AccelerationModule") {
				modConfig = std::make_unique<AccelerationModuleConfig>();
			} else if(name == "DragModule") {
				modConfig = std::make_unique<DragModuleConfig>();
			} else if(name == "SizeOverLifetimeModule") {
				modConfig = std::make_unique<SizeOverLifetimeConfig>();
			} else if(name == "TextureSheetAnimationModule") {
				modConfig = std::make_unique<TextureSheetAnimationConfig>();
			} else if(name == "OverLifetimeModule") {
				modConfig = std::make_unique<OverLifetimeModuleConfig>();
			} else if(name == "ColorOverLifetimeModule") {
				modConfig = std::make_unique<LifetimeModuleConfig>(name, LifetimeModuleTarget::Color);
			} else if(name == "AlphaOverLifetimeModule") {
				modConfig = std::make_unique<LifetimeModuleConfig>(name, LifetimeModuleTarget::Alpha);
			} else if(name == "SizeCurveOverLifetimeModule") {
				modConfig = std::make_unique<LifetimeModuleConfig>(name, LifetimeModuleTarget::Size);
			} else if(name == "RotationOverLifetimeModule") {
				modConfig = std::make_unique<LifetimeModuleConfig>(name, LifetimeModuleTarget::Rotation);
			} else if(name == "VelocityOverLifetimeModule") {
				modConfig = std::make_unique<LifetimeModuleConfig>(name, LifetimeModuleTarget::Velocity);
			} else if(name == "EmissiveOverLifetimeModule") {
				modConfig = std::make_unique<LifetimeModuleConfig>(name, LifetimeModuleTarget::Emissive);
			}

			if(!modConfig) {
				return nullptr;
			}

			modConfig->FromJson(j);
			return modConfig;
		}

	} // namespace ModuleConfigFactory

} // namespace CalyxEngine
