#pragma once

#include <Data/Engine/Configs/Scene/Objects/Particle/Module/ModuleConfig.h>
#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>

namespace CalyxEngine {

	// Lifetime CurveをCPU Particleへ適用するRuntime Module。編集データや描画リソースは所有しない。
	class LifetimeModule final : public BaseFxModule {
	public:
		explicit LifetimeModule(const LifetimeModuleConfig& config);

		void ShowGuiContent() override;
		void OnUpdate(FxUnit& particle, float deltaTime) override;
		const char* GetObjectClassName() const override { return typeName_.c_str(); }

		const LifetimeModuleConfig& GetConfig() const { return config_; }

	private:
		LifetimeModuleConfig config_;
		std::string typeName_;
	};

} // namespace CalyxEngine
