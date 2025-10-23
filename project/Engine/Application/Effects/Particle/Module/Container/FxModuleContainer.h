#pragma once

// engine
#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/EmitterConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/Module/ModuleConfig.h>

// std
#include <vector>
#include <memory>
#include <string>
#include <algorithm>

/// <summary>
/// エフェクトモジュールコンテナ
/// </summary>
class FxModuleContainer{
public:
	//===================================================================*/
	//				public methods
	//===================================================================*/
	FxModuleContainer() = default;
	FxModuleContainer(const std::vector<std::unique_ptr<BaseModuleConfig>>& moduleConfigs);

	/// <summary>
	/// モジュール追加
	/// </summary>
	/// <param name="モジュール名"></param>
	/// <param name="フラグ"></param>
	void AddModule(const std::string& name, bool enabled = true);

	/// <summary>
	/// モジュール削除
	/// </summary>
	/// <param name="モジュール名"></param>
	void RemoveModule(const std::string& name);

	/// <summary>
	/// モジュールがあるか
	/// </summary>
	/// <param name="モジュール名"></param>
	/// <returns></returns>
	bool HasModule(const std::string& name) const;


	/// <summary>
	/// モジュールの適用切り替え
	/// </summary>
	/// <param name="モジュール名"></param>
	/// <param name="フラグ"></param>
	void SetModuleEnabled(const std::string& name, bool enabled);

	const std::vector<std::unique_ptr<BaseFxModule>>& GetModules() const{ return modules_; }

	// GUI
	void ShowModulesGui();
	void ShowAvailableModulesGui();

	// Config serialization
	void ApplyConfigs(const std::vector<std::unique_ptr<BaseModuleConfig>>& configs);
	std::vector<std::unique_ptr<BaseModuleConfig>> ExtractConfigs() const;

private:
	//===================================================================*/
	//				private methods
	//===================================================================*/
	std::vector<std::unique_ptr<BaseFxModule>> modules_;
};
