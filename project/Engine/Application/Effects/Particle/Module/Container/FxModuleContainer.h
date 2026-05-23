#pragma once

#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/EmitterConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/Module/ModuleConfig.h>

#include <vector>
#include <memory>
#include <string>

namespace CalyxEngine {
	/* ========================================================================
	/*		エフェクトモジュールコンテナ
	/* ===================================================================== */
	class FxModuleContainer{
	public:
		FxModuleContainer() = default;
		FxModuleContainer(const std::vector<std::unique_ptr<CalyxEngine::BaseModuleConfig>>& moduleConfigs);

		/// <summary>
		/// 追加
		/// </summary>
		/// <param name="name"></param>
		/// <param name="enabled"></param>
		void AddModule(const std::string& name, bool enabled = true);

		/// <summary>
		/// 削除
		/// </summary>
		/// <param name="name"></param>
		void RemoveModule(const std::string& name);

		//--------- GUI -----------------------------------------------------
		/// <summary>
		/// モジュールのパラメータ調整
		/// </summary>
		void ShowModulesGui();

		/// <summary>
		/// 追加されていないモジュールの追加gui
		/// </summary>
		void ShowAvailableModulesGui();

		//--------- config -----------------------------------------------------
		// 適用
		void ApplyConfigs(const std::vector<std::unique_ptr<CalyxEngine::BaseModuleConfig>>& configs);
		// 掃き出し
		std::vector<std::unique_ptr<CalyxEngine::BaseModuleConfig>> ExtractConfigs() const;

		//--------- accessor -----------------------------------------------------
		const std::vector<std::unique_ptr<BaseFxModule>>& GetModules() const{ return modules_; }
		bool HasModule(const std::string& name) const;
		void SetModuleEnabled(const std::string& name, bool enabled);

	private:
		std::vector<std::unique_ptr<BaseFxModule>> modules_;
	};
}