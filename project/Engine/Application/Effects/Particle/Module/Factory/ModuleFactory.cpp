#include "ModuleFactory.h"

// module
#include <Engine/Application/Effects/Particle/Module/Velocity/GravityModule.h>
#include <Engine/Application/Effects/Particle/Module/Size/SizeOverLiftimeModule.h>
#include <Engine/Application/Effects/Particle/Module/Uv/TextureSheetAnimModule.h>
#include <Engine/Application/Effects/Particle/Module/OverLifetime/OverLifetimeModule.h>

namespace CalyxEngine {
	namespace FxModuleFactory {

		//------------------------------------------------------------------------------
		// Config -> Module
		//   文字列名による型判定を廃止し、dynamic_cast で安全に分岐
		//------------------------------------------------------------------------------
		std::unique_ptr<CalyxEngine::BaseFxModule> CreateFromConfig(const CalyxEngine::BaseModuleConfig& config) {
			// Gravity
			if (auto* c = dynamic_cast<const GravityModuleConfig*>(&config)) {
				auto m = std::make_unique<GravityModule>(c->name);
				m->SetEnabled(c->enabled);
				m->SetGravity(c->gravity);
				return m;
			}
			// SizeOverLifetime（既存の綴りに合わせる）
			if (auto* c = dynamic_cast<const SizeOverLifetimeConfig*>(&config)) {
				auto m = std::make_unique<SizeOverLiftimeModule>(c->name);
				m->SetEnabled(c->enabled);
				m->SetIsGrowing(c->isGrowing);
				m->SetEaseType(c->easeType);
				return m;
			}
			// TextureSheetAnimation
			if (auto* c = dynamic_cast<const TextureSheetAnimationConfig*>(&config)) {
				auto m = std::make_unique<CalyxEngine::TextureSheetAnimationModule>(c->name);
				m->SetEnabled(c->enabled);
				m->UseGridMode(c->rows, c->cols);
				m->SetLoop(c->loop);
				m->SetAnimationSpeed(c->animationSpeed);
				m->SetUseCustomFrames(c->useCustomFrames);
				return m;
			}
			// OverLifetime
			if (auto* c = dynamic_cast<const CalyxEngine::OverLifetimeModuleConfig*>(&config)) {
				auto m = std::make_unique<CalyxEngine::OverLifetimeModule>(c->name);
				c->ApplyTo(*m);
				return m;
			}

			// 未知のConfig
			return nullptr;
		}

		//------------------------------------------------------------------------------
		// Module -> Config
		//   こちらも dynamic_cast ベース。module.GetName() では判定しない！
		//------------------------------------------------------------------------------
		std::unique_ptr<CalyxEngine::BaseModuleConfig> CreateConfigFromModule(const CalyxEngine::BaseFxModule& module) {
			// Gravity
			if (auto* m = dynamic_cast<const GravityModule*>(&module)) {
				auto cfg = std::make_unique<GravityModuleConfig>();
				cfg->name    = m->GetName();
				cfg->enabled = m->IsEnabled();
				cfg->gravity = m->GetGravity();
				return cfg;
			}
			// SizeOverLifetime
			if (auto* m = dynamic_cast<const SizeOverLiftimeModule*>(&module)) {
				auto cfg       = std::make_unique<SizeOverLifetimeConfig>();
				cfg->name      = m->GetName();
				cfg->enabled   = m->IsEnabled();
				cfg->isGrowing = m->GetIsGrowing();
				cfg->easeType  = m->GetEaseType();
				return cfg;
			}
			// TextureSheetAnimation
			if (auto* m = dynamic_cast<const CalyxEngine::TextureSheetAnimationModule*>(&module)) {
				auto cfg               = std::make_unique<TextureSheetAnimationConfig>();
				cfg->name              = m->GetName();
				cfg->enabled           = m->IsEnabled();
				cfg->rows              = m->GetRows();
				cfg->cols              = m->GetCols();
				cfg->loop              = m->GetLoop();
				cfg->animationSpeed    = m->GetAnimationSpeed();
				cfg->useCustomFrames   = m->GetUseCustomFrames();
				return cfg;
			}
			// OverLifetime
			if (auto* m = dynamic_cast<const CalyxEngine::OverLifetimeModule*>(&module)) {
				auto cfg = std::make_unique<CalyxEngine::OverLifetimeModuleConfig>();
				cfg->ExtractFrom(*m);
				return cfg;
			}

			// 未知のModule
			return nullptr;
		}

		//------------------------------------------------------------------------------
		// CreateByName : UIの「追加」から型名で生成
		//------------------------------------------------------------------------------
		std::unique_ptr<CalyxEngine::BaseFxModule> CreateByName(const std::string& typeName) {
			if (typeName == "GravityModule") {
				return std::make_unique<GravityModule>("GravityModule");
			}
			if (typeName == "SizeOverLifetimeModule") {
				return std::make_unique<SizeOverLiftimeModule>("OverLifetimeModule");
			}
			if (typeName == "TextureSheetAnimationModule") {
				return std::make_unique<CalyxEngine::TextureSheetAnimationModule>("TextureSheetAnimationModule");
			}
			if (typeName == "OverLifetimeModule") { 
				return std::make_unique<CalyxEngine::OverLifetimeModule>("OverLifetimeModule");
			}
			return nullptr;
		}

	} // namespace FxModuleFactory
}