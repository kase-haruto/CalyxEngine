#include "FxModuleContainer.h"

#include <Engine/Application/Effects/Particle/Module/Factory/ModuleFactory.h>
#include <algorithm>
#include <externals/imgui/imgui.h>

namespace CalyxEngine {
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

		const char* GetModuleDescription(const std::string& typeName) {
			if(typeName == "GravityModule")
				return "重力（加速度）を毎フレーム速度へ加算します。\nStrengthでXYZ方向の強さを設定できます。";
			if(typeName == "AccelerationModule")
				return "一定の加速度を毎フレーム速度へ加算します。\nXYZ方向ごとに加速量を設定できます。";
			if(typeName == "DragModule")
				return "速度を時間に応じて滑らかに減衰させます。\nDragが大きいほど素早く減速します。";
			if(typeName == "SizeOverLifetimeModule")
				return "寿命の進行度に合わせてパーティクルを拡大または縮小します。\n成長方向とイージングを設定できます。";
			if(typeName == "TextureSheetAnimationModule")
				return "テクスチャを行・列のグリッドに分割し、フレームアニメーションさせます。\nループと再生速度（fps）を設定できます。";
			if(typeName == "OverLifetimeModule")
				return "寿命の進行度に合わせてScale・Rotation・Color・Alphaを補間します。\n対象、合成方法、イージング、開始値・終了値を設定できます。";
			if(typeName == "ColorOverLifetimeModule")
				return "寿命に合わせてRGBAカラーをグラデーション変化させます。\n開始色と終了色を設定できます。";
			if(typeName == "AlphaOverLifetimeModule")
				return "寿命に合わせて透明度を変化させます。\n定数、カーブ、ランダム範囲を選択できます。";
			if(typeName == "SizeCurveOverLifetimeModule")
				return "寿命に合わせてXYZサイズを個別に変化させます。\n各軸でカーブやランダム範囲を設定できます。";
			if(typeName == "RotationOverLifetimeModule")
				return "寿命に合わせてXYZ回転値を変化させます。\n各軸でカーブやランダム範囲を設定できます。";
			if(typeName == "VelocityOverLifetimeModule")
				return "寿命に合わせてXYZ速度を変化させます。\n現在速度をカーブの評価値で置き換えます。";
			if(typeName == "EmissiveOverLifetimeModule")
				return "寿命に合わせて発光強度と発光色を変化させます。\n強度カーブと開始・終了色を設定できます。";
			return "";
		}

	} // namespace

	// =============================================================
	// コンフィグ → モジュール構築
	// =============================================================
	FxModuleContainer::FxModuleContainer(
		const std::vector<std::unique_ptr<CalyxEngine::BaseModuleConfig>>& moduleConfigs) { ApplyConfigs(moduleConfigs); }

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
		RebuildExecutionPlan();
	}

	// =============================================================
	// 削除
	// =============================================================
	void FxModuleContainer::RemoveModule(const std::string& displayName) {
		modules_.erase(
			std::remove_if(modules_.begin(), modules_.end(),
						   [&](const std::unique_ptr<BaseFxModule>& m) { return m->GetName() == displayName; }),
			modules_.end());
		RebuildExecutionPlan();
	}

	// =============================================================
	// config → module 復元（JSON Load 時）
	// =============================================================
	void FxModuleContainer::ApplyConfigs(
		const std::vector<std::unique_ptr<CalyxEngine::BaseModuleConfig>>& configs) {
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
		RebuildExecutionPlan();
	}

	void FxModuleContainer::RebuildExecutionPlan() {
		initializeModules_.clear();
		updateModules_.clear();
		initializeModules_.reserve(modules_.size());
		updateModules_.reserve(modules_.size());
		for(const auto& module : modules_) {
			const ParticleModuleStage stage = module->GetStage();
			if(stage == ParticleModuleStage::Spawn || stage == ParticleModuleStage::Initialize)
				initializeModules_.push_back(module.get());
			else if(stage == ParticleModuleStage::Update)
				updateModules_.push_back(module.get());
		}
	}

	// =============================================================
	// module → config（JSON Save 時）
	// =============================================================
	std::vector<std::unique_ptr<CalyxEngine::BaseModuleConfig>>
	FxModuleContainer::ExtractConfigs() const {
		std::vector<std::unique_ptr<CalyxEngine::BaseModuleConfig>> result;

		for(const auto& mod : modules_) {
			auto cfg = FxModuleFactory::CreateConfigFromModule(*mod);
			if(!cfg) continue;

			cfg->guid = mod->GetGuid();
			cfg->name = mod->GetObjectClassName();

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

			ImGui::Indent();
			bool remove = ImGui::SmallButton("Remove");
			ImGui::Unindent();

			if(open && enabled) {
				ImGui::Indent();
				m->ShowGuiContent();
				ImGui::Unindent();
			}

			if(remove) {
				it = modules_.erase(it);
				RebuildExecutionPlan();
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
			"AccelerationModule",
			"DragModule",
			"SizeOverLifetimeModule",
			"TextureSheetAnimationModule",
			"OverLifetimeModule",
			"ColorOverLifetimeModule",
			"AlphaOverLifetimeModule",
			"SizeCurveOverLifetimeModule",
			"RotationOverLifetimeModule",
			"VelocityOverLifetimeModule",
			"EmissiveOverLifetimeModule",
		};

		for(const auto& typeName : allModules) {
			if(!CanHaveMultipleInstances(typeName) && HasModuleOfType(modules_, typeName))
				continue;

			if(ImGui::Button(typeName.c_str()))
				AddModule(typeName);

			if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("%s", GetModuleDescription(typeName));
			}
		}
	}
} // namespace CalyxEngine
