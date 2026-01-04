#pragma once

#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/EmitterConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/Module/ModuleConfig.h>

#include <vector>
#include <memory>
#include <string>
#include <algorithm>

class FxModuleContainer{
public:
	FxModuleContainer() = default;

	FxModuleContainer(const std::vector<std::unique_ptr<BaseModuleConfig>>& moduleConfigs);

	void AddModule(const std::string& name, bool enabled = true);
	void RemoveModule(const std::string& name);
	bool HasModule(const std::string& name) const;
	void SetModuleEnabled(const std::string& name, bool enabled);

	const std::vector<std::unique_ptr<BaseFxModule>>& GetModules() const{ return modules_; }

	// GUI
	void ShowModulesGui();
	void ShowAvailableModulesGui();

	// Config serialization
	void ApplyConfigs(const std::vector<std::unique_ptr<BaseModuleConfig>>& configs);
	std::vector<std::unique_ptr<BaseModuleConfig>> ExtractConfigs() const;

private:
	std::vector<std::unique_ptr<BaseFxModule>> modules_;
};
