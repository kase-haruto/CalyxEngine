#pragma once

#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>

#include <Data/Engine/Configs/Scene/Objects/Particle/Module/ModuleConfig.h>

#include <memory>
#include <string>
namespace CalyxEngine {
	/* ========================================================================
	/*		モジュール作成クラス
	/* ===================================================================== */
	namespace FxModuleFactory {
		/// <summary>
		/// コンフィグから作成
		/// </summary>
		/// <param name="config"></param>
		/// <returns></returns>
		std::unique_ptr<CalyxEngine::BaseFxModule> CreateFromConfig(const CalyxEngine::BaseModuleConfig& config);

		/// <summary>
		/// コンフィグをモジュールから作成
		/// </summary>
		/// <param name="module"></param>
		/// <returns></returns>
		std::unique_ptr<CalyxEngine::BaseModuleConfig> CreateConfigFromModule(const CalyxEngine::BaseFxModule& module);

		/// <summary>
		/// 名前からモジュールを作成
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		std::unique_ptr<CalyxEngine::BaseFxModule> CreateByName(const std::string& name);
	}
}
