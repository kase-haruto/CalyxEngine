#include "ModuleFactory.h"
#include <Engine/Application/Effects/Particle/Module/Velocity/GravityModule.h>
#include <Engine/Application/Effects/Particle/Module/Size/SizeOverLiftimeModule.h>
#include <Engine/Application/Effects/Particle/Module/Uv/TextureSheetAnimModule.h>

namespace FxModuleFactory{

	enum class FxModuleType{
		Gravity,
		SizeOverLifetime,
		TextureSheetAnimation,
		Unknown
	};

	// モジュール名からタイプを判定
	FxModuleType GetModuleType(const std::string& name){
		if (name == "GravityModule") return FxModuleType::Gravity;
		if (name == "SizeOverLifetimeModule") return FxModuleType::SizeOverLifetime;
		if (name == "TextureSheetAnimationModule") return FxModuleType::TextureSheetAnimation;
		return FxModuleType::Unknown;
	}

	// コンフィグからモジュールを生成
	std::unique_ptr<BaseFxModule> CreateFromConfig(const BaseModuleConfig& config){
		switch (GetModuleType(config.name)){
			case FxModuleType::Gravity:
			{
				const auto* gConfig = dynamic_cast< const GravityModuleConfig* >(&config);
				if (!gConfig) return nullptr;
				auto mod = std::make_unique<GravityModule>(gConfig->name);
				mod->SetEnabled(gConfig->enabled);
				mod->SetGravity(gConfig->gravity);
				return mod;
			}
			case FxModuleType::SizeOverLifetime:
			{
				const auto* sConfig = dynamic_cast< const SizeOverLifetimeConfig* >(&config);
				if (!sConfig) return nullptr;
				auto mod = std::make_unique<SizeOverLiftimeModule>(sConfig->name);
				mod->SetEnabled(sConfig->enabled);
				mod->SetIsGrowing(sConfig->isGrowing);
				mod->SetEaseType(sConfig->easeType);
				return mod;
			}
			case FxModuleType::TextureSheetAnimation:
			{
				const auto* sConfig = dynamic_cast< const TextureSheetAnimationConfig* >(&config);
				if (!sConfig) return nullptr;
				auto mod = std::make_unique<TextureSheetAnimationModule>(sConfig->name);
				mod->SetEnabled(sConfig->enabled);
				mod->UseGridMode(sConfig->rows, sConfig->cols);
				mod->SetLoop(sConfig->loop);
				mod->SetAnimationSpeed(sConfig->animationSpeed);
				mod->SetUseCustomFrames(sConfig->useCustomFrames);
				return mod;
			}
			default:
				return nullptr;
		}
	}

	// モジュールからコンフィグを生成
	std::unique_ptr<BaseModuleConfig> CreateConfigFromModule(const BaseFxModule& module){
		switch (GetModuleType(module.GetName())){
			case FxModuleType::Gravity:
			{
				const auto& mod = static_cast< const GravityModule& >(module);
				auto config = std::make_unique<GravityModuleConfig>();
				config->enabled = mod.IsEnabled();
				config->gravity = mod.GetGravity();
				return config;
			}
			case FxModuleType::SizeOverLifetime:
			{
				const auto& mod = static_cast< const SizeOverLiftimeModule& >(module);
				auto config = std::make_unique<SizeOverLifetimeConfig>();
				config->enabled = mod.IsEnabled();
				config->isGrowing = mod.GetIsGrowing();
				config->easeType = mod.GetEaseType();
				return config;
			}
			case FxModuleType::TextureSheetAnimation:
			{
				const auto& mod = static_cast< const TextureSheetAnimationModule& >(module);
				auto config = std::make_unique<TextureSheetAnimationConfig>();
				config->enabled = mod.IsEnabled();
				config->rows = mod.GetRows();
				config->cols = mod.GetCols();
				config->loop = mod.GetLoop();
				config->animationSpeed = mod.GetAnimationSpeed();
				config->useCustomFrames = mod.GetUseCustomFrames();
				return config;
			}
			default:
				return nullptr;
		}
	}

	//名前から生成
	std::unique_ptr<BaseFxModule> CreateByName(const std::string& name){
		using enum FxModuleType;
		switch (GetModuleType(name)){
			case Gravity:
				return std::make_unique<GravityModule>(name);
			case SizeOverLifetime:
				return std::make_unique<SizeOverLiftimeModule>(name);
			case TextureSheetAnimation:
				return std::make_unique<TextureSheetAnimationModule>(name);
			default:
				return nullptr;
		}
	}
}
