#pragma once

#include <Data/Engine/Configs/Scene/Objects/Particle/EmitterConfig.h>
#include <Engine/Foundation/Math/Vector4.h>

namespace CalyxEngine {
	class FxEmitter;

	struct ThrusterSettings {
		Vector4 innerColor{0.45f, 0.95f, 1.0f, 0.9f};
		Vector4 outerColor{0.85f, 0.18f, 1.0f, 0.32f};
		float length = 2.4f;
		float width = 0.38f;
		float noiseStrength = 0.18f;
		float emission = 7.0f;
	};

	// 既存の加算パーティクルエミッターを2層組み合わせた、スタイライズ表現用スラスター。
	// 内側の層は高輝度なBloomの芯を作り、外側の層は柔らかな色付きの輪郭を作る。
	// 実際の描画責務はParticleRendererに集約したままとする。
	class ThrusterEffect {
	public:
		ThrusterSettings& GetSettings() { return settings_; }
		const ThrusterSettings& GetSettings() const { return settings_; }

		EmitterConfig BuildInnerConfig() const;
		EmitterConfig BuildOuterConfig() const;
		void Apply(FxEmitter& inner, FxEmitter& outer) const;
		bool ShowImGui(const char* label = "Thruster");

	private:
		EmitterConfig BuildLayer(bool inner) const;
		ThrusterSettings settings_{};
	};
}
