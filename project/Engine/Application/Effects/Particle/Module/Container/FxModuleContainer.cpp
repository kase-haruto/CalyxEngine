#include "FxModuleContainer.h"

#include <Engine/Application/Effects/Particle/Module/Factory/ModuleFactory.h>
#include <algorithm>
#include <externals/imgui/imgui.h>

namespace CalyxEffect {
	namespace {

		// 一意な表示名を作る（UI Only）
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

		// この型は複数インスタンスを持てるか？
		bool CanHaveMultipleInstances(const std::string& typeName) {
			if(typeName == "OverLifetimeModule") return true;
			return false;
		}

		// 型名ベースの重複チェック（UI名も考慮）
		bool HasModuleOfType(
			const std::vector<std::unique_ptr<BaseFxModule>>& mods,
			const std::string&								  typeName) {
			for(const auto& m : mods) {
				const std::string& n = m->GetName();
				if(n == typeName) return true;
				if(n.rfind(typeName + " (", 0) == 0) return true;
			}
			return false;
		}

	} // namespace

	// =============================================================
	// コンフィグ → モジュール構築
	// =============================================================
	FxModuleContainer::FxModuleContainer(
		const std::vector<std::unique_ptr<CalyxEffect::BaseModuleConfig>>& moduleConfigs) { ApplyConfigs(moduleConfigs); }

	// =============================================================
	// （GUID を生成する場所）
	// =============================================================
	void FxModuleContainer::AddModule(const std::string& typeName, bool enabled) {
		// 複数不可の型は既にあるなら追加しない
		if(!CanHaveMultipleInstances(typeName) && HasModuleOfType(modules_, typeName))
			return;

		auto module = FxModuleFactory::CreateByName(typeName);
		if(!module) return;

		Guid guid = Guid::New();
		module->SetGuid(guid);

		module->SetEnabled(enabled);

		// UI 表示名だけユニーク扱い
		module->SetName(MakeUniqueName(modules_, typeName));

		modules_.emplace_back(std::move(module));
	}

	// =============================================================
	// 削除
	// =============================================================
	void FxModuleContainer::RemoveModule(const std::string& displayName) {
		modules_.erase(
			std::remove_if(modules_.begin(), modules_.end(),
						   [&](const std::unique_ptr<BaseFxModule>& m) { return m->GetName() == displayName; }),
			modules_.end());
	}

	// =============================================================
	// config → module 復元（JSON Load 時）
	// =============================================================
	void FxModuleContainer::ApplyConfigs(
		const std::vector<std::unique_ptr<CalyxEffect::BaseModuleConfig>>& configs) {
		modules_.clear();

		for(const auto& cfg : configs) {
			auto mod = FxModuleFactory::CreateFromConfig(*cfg);
			if(!mod) continue;

			// ★ GUID を config から完全に復元 ★
			mod->SetGuid(cfg->guid);

			// UI 表示名はユニーク化
			mod->SetName(MakeUniqueName(modules_, cfg->name));

			modules_.emplace_back(std::move(mod));
		}
	}

	// =============================================================
	// module → config（JSON Save 時）
	// =============================================================
	std::vector<std::unique_ptr<CalyxEffect::BaseModuleConfig>>
	FxModuleContainer::ExtractConfigs() const {
		std::vector<std::unique_ptr<CalyxEffect::BaseModuleConfig>> result;

		for(const auto& mod : modules_) {
			auto cfg = FxModuleFactory::CreateConfigFromModule(*mod);
			if(!cfg) continue;

			cfg->guid = mod->GetGuid();
			cfg->name = mod->GetTypeName();

			result.push_back(std::move(cfg));
		}
		return result;
	}

	// =============================================================
	// GUI 表示
	// =============================================================
	void FxModuleContainer::ShowModulesGui() {
		for(auto it = modules_.begin(); it != modules_.end();) {
			auto& m = *it;
			ImGui::PushID(m.get());

			bool enabled = m->IsEnabled();
			if(ImGui::Checkbox("##enabled", &enabled))
				m->SetEnabled(enabled);

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

	// =============================================================
	// 追加リスト GUI
	// =============================================================
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
			if(!CanHaveMultipleInstances(typeName) && HasModuleOfType(modules_, typeName))
				continue;

			if(ImGui::Button(typeName.c_str()))
				AddModule(typeName);
		}
	}
} // namespace CalyxEffect
