#include "ModuleFactory.h"

// module
#include <Engine/Application/Effects/Particle/Module/Velocity/GravityModule.h>
#include <Engine/Application/Effects/Particle/Module/Velocity/MovementModules.h>
#include <Engine/Application/Effects/Particle/Module/Size/SizeOverLiftimeModule.h>
#include <Engine/Application/Effects/Particle/Module/Uv/TextureSheetAnimModule.h>
#include <Engine/Application/Effects/Particle/Module/OverLifetime/OverLifetimeModule.h>
#include <Engine/Application/Effects/Particle/Module/OverLifetime/LifetimeModule.h>

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
			if(auto* c = dynamic_cast<const AccelerationModuleConfig*>(&config)) {
				auto module = std::make_unique<AccelerationModule>(c->name);
				module->SetEnabled(c->enabled);
				module->SetAcceleration(c->acceleration);
				return module;
			}
			if(auto* c = dynamic_cast<const DragModuleConfig*>(&config)) {
				auto module = std::make_unique<DragModule>(c->name);
				module->SetEnabled(c->enabled);
				module->SetDrag(c->drag);
				return module;
			}
			if(auto* c = dynamic_cast<const CalyxEngine::LifetimeModuleConfig*>(&config)) {
				return std::make_unique<CalyxEngine::LifetimeModule>(*c);
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
			if(auto* m = dynamic_cast<const AccelerationModule*>(&module)) {
				auto cfg = std::make_unique<AccelerationModuleConfig>();
				cfg->enabled = m->IsEnabled();
				cfg->acceleration = m->GetAcceleration();
				return cfg;
			}
			if(auto* m = dynamic_cast<const DragModule*>(&module)) {
				auto cfg = std::make_unique<DragModuleConfig>();
				cfg->enabled = m->IsEnabled();
				cfg->drag = m->GetDrag();
				return cfg;
			}
			if(auto* m = dynamic_cast<const CalyxEngine::LifetimeModule*>(&module)) {
				auto cfg = std::make_unique<CalyxEngine::LifetimeModuleConfig>(m->GetConfig());
				cfg->enabled = m->IsEnabled();
				cfg->guid = m->GetGuid();
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
			if(typeName == "AccelerationModule") return std::make_unique<AccelerationModule>();
			if(typeName == "DragModule") return std::make_unique<DragModule>();
			if(typeName == "ColorOverLifetimeModule") return std::make_unique<CalyxEngine::LifetimeModule>(CalyxEngine::LifetimeModuleConfig(typeName, CalyxEngine::LifetimeModuleTarget::Color));
			if(typeName == "AlphaOverLifetimeModule") return std::make_unique<CalyxEngine::LifetimeModule>(CalyxEngine::LifetimeModuleConfig(typeName, CalyxEngine::LifetimeModuleTarget::Alpha));
			if(typeName == "SizeCurveOverLifetimeModule") return std::make_unique<CalyxEngine::LifetimeModule>(CalyxEngine::LifetimeModuleConfig(typeName, CalyxEngine::LifetimeModuleTarget::Size));
			if(typeName == "RotationOverLifetimeModule") return std::make_unique<CalyxEngine::LifetimeModule>(CalyxEngine::LifetimeModuleConfig(typeName, CalyxEngine::LifetimeModuleTarget::Rotation));
			if(typeName == "VelocityOverLifetimeModule") return std::make_unique<CalyxEngine::LifetimeModule>(CalyxEngine::LifetimeModuleConfig(typeName, CalyxEngine::LifetimeModuleTarget::Velocity));
			if(typeName == "EmissiveOverLifetimeModule") return std::make_unique<CalyxEngine::LifetimeModule>(CalyxEngine::LifetimeModuleConfig(typeName, CalyxEngine::LifetimeModuleTarget::Emissive));
			return nullptr;
		}

	} // namespace FxModuleFactory
}
