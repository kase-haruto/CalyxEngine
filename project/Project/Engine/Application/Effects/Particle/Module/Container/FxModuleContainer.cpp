#include "FxModuleContainer.h"

#include <Engine/Application/Effects/Particle/Module/Factory/ModuleFactory.h>
#include <externals/imgui/imgui.h>

FxModuleContainer::FxModuleContainer(const std::vector<std::unique_ptr<BaseModuleConfig>>& moduleConfigs){
	ApplyConfigs(moduleConfigs);
}

void FxModuleContainer::AddModule(const std::string& name, bool enabled){
	if (HasModule(name)) return;
	auto module = FxModuleFactory::CreateByName(name);
	if (module){
		module->SetEnabled(enabled);
		modules_.emplace_back(std::move(module));
	}
}

void FxModuleContainer::RemoveModule(const std::string& name){
	modules_.erase(
		std::remove_if(modules_.begin(), modules_.end(),
		[&] (const std::unique_ptr<BaseFxModule>& m){ return m->GetName() == name; }),
		modules_.end()
	);
}

bool FxModuleContainer::HasModule(const std::string& name) const{
	for (const auto& m : modules_){
		if (m->GetName() == name) return true;
	}
	return false;
}

void FxModuleContainer::SetModuleEnabled(const std::string& name, bool enabled){
	for (auto& m : modules_){
		if (m->GetName() == name){
			m->SetEnabled(enabled);
			break;
		}
	}
}

void FxModuleContainer::ApplyConfigs(const std::vector<std::unique_ptr<BaseModuleConfig>>& configs){
	modules_.clear();
	for (const auto& cfg : configs){
		if (!cfg) continue;
		auto mod = FxModuleFactory::CreateFromConfig(*cfg);
		if (mod){
			modules_.emplace_back(std::move(mod));
		}
	}
}

std::vector<std::unique_ptr<BaseModuleConfig>> FxModuleContainer::ExtractConfigs() const{
	std::vector<std::unique_ptr<BaseModuleConfig>> result;
	for (const auto& mod : modules_){
		auto cfg = FxModuleFactory::CreateConfigFromModule(*mod);
		if (cfg) result.push_back(std::move(cfg));
	}
	return result;
}

void FxModuleContainer::ShowModulesGui(){
	for (auto it = modules_.begin(); it != modules_.end();){
		auto& m = *it;
		ImGui::PushID(m.get());

		bool enabled = m->IsEnabled();
		if (ImGui::Checkbox("##enabled", &enabled)){
			m->SetEnabled(enabled);
		}
		ImGui::SameLine();

		bool open = ImGui::CollapsingHeader(m->GetName().c_str());
		if (open && enabled){
			ImGui::Indent();
			m->ShowGuiContent();
			ImGui::Unindent();
		}

		ImGui::SameLine();
		if (ImGui::Button("Remove")){
			it = modules_.erase(it);
			ImGui::PopID();
			continue;
		}
		ImGui::PopID();
		++it;
	}
}

void FxModuleContainer::ShowAvailableModulesGui(){
	ImGui::Spacing();
	ImGui::SeparatorText("Add Modules");
	static const std::vector<std::string> allModules = {
		"GravityModule",
		"SizeOverLifetimeModule",
		"TextureSheetAnimationModule",
	};

	for (const auto& modName : allModules){
		if (!HasModule(modName)){
			if (ImGui::Button(modName.c_str())){
				AddModule(modName);
			}
		}
	}
}
