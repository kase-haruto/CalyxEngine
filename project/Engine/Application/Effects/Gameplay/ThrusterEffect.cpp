#include "ThrusterEffect.h"

#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <externals/imgui/imgui.h>
#include <algorithm>
#include <memory>

namespace CalyxEngine {
	namespace {
		std::unique_ptr<LifetimeModuleConfig> MakeAlphaModule(float peakAlpha) {
			auto module = std::make_unique<LifetimeModuleConfig>("AlphaOverLifetimeModule", LifetimeModuleTarget::Alpha);
			module->floatCurve.mode = CurveValueMode::Curve;
			module->floatCurve.keys = {{0.0f, 0.0f}, {0.12f, peakAlpha}, {0.72f, peakAlpha * 0.65f}, {1.0f, 0.0f}};
			return module;
		}

		std::unique_ptr<LifetimeModuleConfig> MakeEmissionModule(const Vector4& color, float intensity) {
			auto module = std::make_unique<LifetimeModuleConfig>("EmissiveOverLifetimeModule", LifetimeModuleTarget::Emissive);
			module->floatCurve.mode = CurveValueMode::Curve;
			module->floatCurve.keys = {{0.0f, intensity}, {0.65f, intensity * 0.72f}, {1.0f, 0.0f}};
			module->gradient.keys = {{0.0f, color}, {1.0f, Vector4{color.x, color.y, color.z, 0.0f}}};
			return module;
		}
	}

	EmitterConfig ThrusterEffect::BuildLayer(bool inner) const {
		const float length = (std::max)(settings_.length, 0.05f);
		const float width = (std::max)(settings_.width, 0.01f);
		const float noise = std::clamp(settings_.noiseStrength, 0.0f, 2.0f);
		const Vector4 color = inner ? settings_.innerColor : settings_.outerColor;

		EmitterConfig config;
		config.blendMode = BlendMode::ADD;
		config.texturePath = "Textures/white1x1.dds";
		config.modelPath = "plane.obj";
		config.color = color;
		config.vertexColor = color;
		config.emitRate = inner ? 0.018f : 0.035f;
		config.billboardMode = BillboardMode::Full;
		config.fixedRandomSeed = false;

		config.scale.mode = FxValueMode::Random;
		const float layerWidth = width * (inner ? 0.42f : 1.0f);
		config.scale.min = {layerWidth * 0.72f, layerWidth * 0.72f, 1.0f};
		config.scale.max = {layerWidth * 1.18f, layerWidth * 1.18f, 1.0f};

		config.lifetime.mode = FxValueMode::Random;
		config.lifetime.min = inner ? 0.14f : 0.24f;
		config.lifetime.max = inner ? 0.26f : 0.42f;

		config.direction.enabled = true;
		config.direction.vector.mode = FxValueMode::Random;
		config.direction.vector.min = {-noise * 0.16f, -1.0f, -noise * 0.08f};
		config.direction.vector.max = { noise * 0.16f, -1.0f,  noise * 0.08f};
		config.direction.speed.mode = FxValueMode::Random;
		config.direction.speed.min = length * (inner ? 4.6f : 3.0f);
		config.direction.speed.max = length * (inner ? 6.0f : 4.4f);

		config.gpuCurlNoiseEnabled = noise > 0.001f;
		config.gpuCurlNoiseFrequency = 1.8f;
		config.gpuCurlNoiseOctaves = 2;
		config.gpuCurlNoiseRoughness = 0.55f;
		config.gpuCurlNoiseLacunarity = 2.0f;
		config.gpuCurlNoiseAmplitude = noise * (inner ? 0.16f : 0.42f);
		config.gpuCurlNoiseScrollSpeed = {0.0f, -0.7f, 0.15f};

		config.modules.push_back(MakeAlphaModule(std::clamp(color.w, 0.0f, 1.0f)));
		config.modules.push_back(MakeEmissionModule(color, (std::max)(settings_.emission, 0.0f) * (inner ? 1.0f : 0.32f)));
		return config;
	}

	EmitterConfig ThrusterEffect::BuildInnerConfig() const { return BuildLayer(true); }
	EmitterConfig ThrusterEffect::BuildOuterConfig() const { return BuildLayer(false); }

	void ThrusterEffect::Apply(FxEmitter& inner, FxEmitter& outer) const {
		inner.ApplyConfigFrom(BuildInnerConfig());
		outer.ApplyConfigFrom(BuildOuterConfig());
	}

	bool ThrusterEffect::ShowImGui(const char* label) {
		bool changed = false;
		if(ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_SpanAvailWidth)) {
			changed |= ImGui::ColorEdit4("Inner Color", &settings_.innerColor.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
			changed |= ImGui::ColorEdit4("Outer Color", &settings_.outerColor.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
			changed |= ImGui::DragFloat("Length", &settings_.length, 0.02f, 0.05f, 20.0f);
			changed |= ImGui::DragFloat("Width", &settings_.width, 0.01f, 0.01f, 5.0f);
			changed |= ImGui::DragFloat("Noise Strength", &settings_.noiseStrength, 0.01f, 0.0f, 2.0f);
			changed |= ImGui::DragFloat("Emission", &settings_.emission, 0.05f, 0.0f, 50.0f);
			ImGui::TreePop();
		}
		return changed;
	}
}
