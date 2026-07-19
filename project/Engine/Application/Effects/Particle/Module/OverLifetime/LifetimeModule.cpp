#include "LifetimeModule.h"

#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <externals/imgui/imgui.h>

namespace CalyxEngine {

	LifetimeModule::LifetimeModule(const LifetimeModuleConfig& config)
		: BaseFxModule(config.name), config_(config), typeName_(config.name) {
		SetEnabled(config.enabled);
		SetGuid(config.guid);
	}

	void LifetimeModule::ShowGuiContent() {
		if(config_.target == LifetimeModuleTarget::Color) {
			if(config_.gradient.keys.size() < 2) {
				config_.gradient.keys = {{0.0f, {1, 1, 1, 1}}, {1.0f, {1, 1, 1, 1}}};
			}
			ImGui::ColorEdit4("Start Color", &config_.gradient.keys.front().color.x);
			ImGui::ColorEdit4("End Color", &config_.gradient.keys.back().color.x);
			return;
		}

		if(config_.target == LifetimeModuleTarget::Size ||
		   config_.target == LifetimeModuleTarget::Rotation ||
		   config_.target == LifetimeModuleTarget::Velocity) {
			FloatCurve* curves[] = {&config_.vectorCurve.x, &config_.vectorCurve.y, &config_.vectorCurve.z};
			const char* labels[] = {"X", "Y", "Z"};
			for(int i = 0; i < 3; ++i) {
				ImGui::PushID(i);
				int mode = static_cast<int>(curves[i]->mode);
				ImGui::Combo("Mode", &mode, "Constant\0Linear\0Curve\0Random Constants\0Random Curves\0");
				curves[i]->mode = static_cast<CurveValueMode>(std::clamp(mode, 0, 4));
				ImGui::DragFloat(labels[i], &curves[i]->constant, 0.01f);
				ImGui::DragFloat("Min", &curves[i]->constantMin, 0.01f);
				ImGui::DragFloat("Max", &curves[i]->constantMax, 0.01f);
				ImGui::PopID();
			}
			return;
		}

		int mode = static_cast<int>(config_.floatCurve.mode);
		ImGui::Combo("Mode", &mode, "Constant\0Linear\0Curve\0Random Constants\0Random Curves\0");
		config_.floatCurve.mode = static_cast<CurveValueMode>(std::clamp(mode, 0, 4));
		ImGui::DragFloat("Value", &config_.floatCurve.constant, 0.01f);
		ImGui::DragFloat("Min", &config_.floatCurve.constantMin, 0.01f);
		ImGui::DragFloat("Max", &config_.floatCurve.constantMax, 0.01f);
		if(config_.target == LifetimeModuleTarget::Emissive) {
			if(config_.gradient.keys.size() < 2) config_.gradient.keys = {{0.0f, {1, 1, 1, 1}}, {1.0f, {1, 1, 1, 1}}};
			ImGui::ColorEdit4("Emissive Start", &config_.gradient.keys.front().color.x);
			ImGui::ColorEdit4("Emissive End", &config_.gradient.keys.back().color.x);
		}
	}

	void LifetimeModule::OnUpdate(FxUnit& particle, float) {
		const float random01 = ParticleRandom01(particle.randomSeed, HashParticleSeed(static_cast<uint32_t>(config_.target)));
		switch(config_.target) {
		case LifetimeModuleTarget::Color: particle.color = config_.gradient.Evaluate(particle.lifeT); break;
		case LifetimeModuleTarget::Alpha: particle.color.w = config_.floatCurve.Evaluate(particle.lifeT, random01); break;
		case LifetimeModuleTarget::Size: particle.scale = particle.initialScale * config_.vectorCurve.Evaluate(particle.lifeT, random01); break;
		case LifetimeModuleTarget::Rotation: particle.rotationEuler = config_.vectorCurve.Evaluate(particle.lifeT, random01); break;
		case LifetimeModuleTarget::Velocity: particle.velocity = config_.vectorCurve.Evaluate(particle.lifeT, random01); break;
		case LifetimeModuleTarget::Emissive:
			particle.emissiveIntensity = config_.floatCurve.Evaluate(particle.lifeT, random01);
			particle.emissiveColor = config_.gradient.Evaluate(particle.lifeT);
			break;
		}
	}

} // namespace CalyxEngine
