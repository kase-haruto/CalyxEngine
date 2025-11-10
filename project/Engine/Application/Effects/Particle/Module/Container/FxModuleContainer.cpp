#include "FxModuleContainer.h"

#include <Engine/Application/Effects/Particle/Module/Factory/ModuleFactory.h>
#include <algorithm> // 竊・霑ｽ蜉
#include <externals/imgui/imgui.h>

namespace {

// 一意な表示名を作る
std::string MakeUniqueName(
	const std::vector<std::unique_ptr<BaseFxModule>>& mods,
	const std::string&								  base) {
	auto exists = [&](const std::string& s) {
		for(auto& m : mods)
			if(m->GetName() == s) return true;
		return false;
	};
	if(!exists(base)) return base;

	int			i = 2;
	std::string n;
	do {
		n = base + " (" + std::to_string(i++) + ")";
	} while(exists(n));
	return n;
}

// 複数インスタンス許可フラグ（OverLifetime は複数OK）
bool CanHaveMultipleInstances(const std::string& typeName) {
	if(typeName == "OverLifetimeModule") return true;
	return false;
}

// 型名ベースで「その型が既にあるか」を判定（"Name (2)" も検出）
bool HasModuleOfType(const std::vector<std::unique_ptr<BaseFxModule>>& mods,
							const std::string&								  typeName) {
	for(const auto& m : mods) {
		const std::string& n = m->GetName();
		if(n == typeName) return true;
		// "TypeName (" で始まる（ユニーク名付与後）なら同型とみなす
		if(n.rfind(typeName + " (", 0) == 0) return true; // starts_with
	}
	return false;
}

} // namespace

FxModuleContainer::FxModuleContainer(const std::vector<std::unique_ptr<BaseModuleConfig>>& moduleConfigs) {
	ApplyConfigs(moduleConfigs);
}

void FxModuleContainer::AddModule(const std::string& name, bool enabled) {
	// 複数不可の型だけ、型名ベースで重複ブロック
	if(!CanHaveMultipleInstances(name) && HasModuleOfType(modules_, name)) return;

	auto module = FxModuleFactory::CreateByName(name);
	if(module) {
		module->SetEnabled(enabled);
		// 表示名の重複を避ける（複数OKの型でもUI上はユニークに）
		module->SetName(MakeUniqueName(modules_, module->GetName()));
		modules_.emplace_back(std::move(module));
	}
}

void FxModuleContainer::RemoveModule(const std::string& name) {
	modules_.erase(
		std::remove_if(modules_.begin(), modules_.end(),
					   [&](const std::unique_ptr<BaseFxModule>& m) { return m->GetName() == name; }),
		modules_.end());
}

bool FxModuleContainer::HasModule(const std::string& name) const {
	for(const auto& m : modules_) {
		if(m->GetName() == name) return true;
	}
	return false;
}

void FxModuleContainer::SetModuleEnabled(const std::string& name, bool enabled) {
	for(auto& m : modules_) {
		if(m->GetName() == name) {
			m->SetEnabled(enabled);
			break;
		}
	}
}

void FxModuleContainer::ApplyConfigs(const std::vector<std::unique_ptr<BaseModuleConfig>>& configs) {
	modules_.clear();
	for(const auto& cfg : configs) {
		if(!cfg) continue;
		auto mod = FxModuleFactory::CreateFromConfig(*cfg);
		if(mod) {
			// JSONから復元した表示名が衝突する可能性に備えユニーク化
			mod->SetName(MakeUniqueName(modules_, mod->GetName()));
			modules_.emplace_back(std::move(mod));
		}
	}
}

std::vector<std::unique_ptr<BaseModuleConfig>> FxModuleContainer::ExtractConfigs() const {
	std::vector<std::unique_ptr<BaseModuleConfig>> result;
	for(const auto& mod : modules_) {
		auto cfg = FxModuleFactory::CreateConfigFromModule(*mod);
		if(cfg) result.push_back(std::move(cfg));
	}
	return result;
}

void FxModuleContainer::ShowModulesGui() {
	for(auto it = modules_.begin(); it != modules_.end();) {
		auto& m = *it;
		ImGui::PushID(m.get());

		bool enabled = m->IsEnabled();
		if(ImGui::Checkbox("##enabled", &enabled)) {
			m->SetEnabled(enabled);
		}
		ImGui::SameLine();

		bool open = ImGui::CollapsingHeader(m->GetName().c_str());
		if(open && enabled) {
			ImGui::Indent();
			m->ShowGuiContent();
			ImGui::Unindent();
		}

		ImGui::SameLine();
		if(ImGui::Button("Remove")) {
			it = modules_.erase(it);
			ImGui::PopID();
			continue;
		}
		ImGui::PopID();
		++it;
	}
}

void FxModuleContainer::ShowAvailableModulesGui() {
	ImGui::Spacing();
	ImGui::SeparatorText("Add Modules");
	static const std::vector<std::string> allModules = {
		"GravityModule",
		"SizeOverLifetimeModule",
		"TextureSheetAnimationModule",
		"OverLifetimeModule",
	};

	for(const auto& typeName : allModules) {
		// 複数不可の型だけ、既に同型があればボタン非表示
		if(!CanHaveMultipleInstances(typeName) && HasModuleOfType(modules_, typeName))
			continue;

		if(ImGui::Button(typeName.c_str())) {
			AddModule(typeName);
		}
	}
}
