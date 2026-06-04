#include "FxObject.h"
/* ========================================================================
/*		include space
/* ===================================================================== */
#include <Engine/Application/Effects/EffectAsset.h>
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Emitter/GpuFxEmitter.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <filesystem>

namespace CalyxEngine {


	namespace {

		std::shared_ptr<ParticleSystemObject>
		FindChildByGuid(const std::vector<std::weak_ptr<ParticleSystemObject>>& list, const Guid& g) {
			for(const auto& wp : list) {
				if(auto sp = wp.lock()) {
					if(sp->GetGuid() == g) {
						return sp;
					}
				}
			}
			return nullptr;
		}

	} // namespace

	/////////////////////////////////////////////////////////////////////////////////////////
	//		ctor / dtor
	/////////////////////////////////////////////////////////////////////////////////////////
	FxObject::FxObject(const std::string& name) {
		SceneObject::SetName(name, ObjectType::Effect);
		config_.SetOnApplied([this](const EffectObjectConfig&) { this->ApplyConfig(); });
		config_.SetOnExtracted([this](const EffectObjectConfig&) { this->ExtractConfig(); });
	}
	FxObject::~FxObject() = default;

	/////////////////////////////////////////////////////////////////////////////////////////
	//		初期化
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxObject::Initialize() {
		// callbacks are set in ctor so Load/Save works immediately
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		更新
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxObject::Update(float dt) { SceneObject::Update(dt); }

	/////////////////////////////////////////////////////////////////////////////////////////
	//		常時更新
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxObject::AlwaysUpdate(float) {
		// 行列の更新
		SyncChildrenFromWorldTransform();
	}

	void FxObject::Destroy() {
		emitters_.clear();

		// 最後に自分を破壊する
		SceneObject::Destroy();
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		再生
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxObject::PlayAll() const {
		// 期限切れを掃除しつつ再生
		auto& selfEmitters = const_cast<std::vector<std::weak_ptr<ParticleSystemObject>>&>(emitters_);
		for(auto it = selfEmitters.begin(); it != selfEmitters.end();) {
			if(auto sp = it->lock()) {
				sp->Play();
				++it;
			} else {
				it = selfEmitters.erase(it);
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		停止
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxObject::StopAll() const {
		auto& selfEmitters = const_cast<std::vector<std::weak_ptr<ParticleSystemObject>>&>(emitters_);
		for(auto it = selfEmitters.begin(); it != selfEmitters.end();) {
			if(auto sp = it->lock()) {
				sp->Stop();
				++it;
			} else {
				it = selfEmitters.erase(it);
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		再再生
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxObject::RestartAll() const {
		for(auto& wp : emitters_) {
			if(auto sp = wp.lock()) {
				sp->ResetRecursive();
			}
		}
		PlayAll();
	}

	void FxObject::SetAlphaMultiplier(float a) {
		for(auto& wp : emitters_) {
			if(auto sp = wp.lock()) {
				sp->SetAlphaMultiplier(a);
			}
		}
	}
	void FxObject::SetCameraFade(float nearZ, float farZ) {
		for(auto& wp : emitters_) {
			if(auto sp = wp.lock()) {
				sp->SetCameraFade(nearZ, farZ);
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		デバッグui
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxObject::ShowGui() {

		// コンフィグのセーブ・ローど
		if(ImGui::Button("Load Effect")) {
			std::filesystem::create_directories(kConfigRoot_);
			IGFD::FileDialogConfig config;
			config.path = kConfigRoot_.string();
			ImGuiFileDialog::Instance()->OpenDialog(
				"EffectLoadDialog",
				"load effect",
				".effect",
				config);
		}
		if(ImGuiFileDialog::Instance()->Display("EffectLoadDialog")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				LoadEffectAsset(ImGuiFileDialog::Instance()->GetFilePathName());
			}
			ImGuiFileDialog::Instance()->Close();
		}
		// ルート Transform
		worldTransform_.ShowImGui();

		// 一括操作
		if(ImGui::Button("Play All")) PlayAll();
		ImGui::SameLine();
		if(ImGui::Button("Stop All")) StopAll();
		ImGui::SameLine();
		if(ImGui::Button("Reset All")) RestartAll();
		if(ImGui::Button("Save Effect Asset")) {
			std::filesystem::create_directories(kConfigRoot_);
			SaveEffectAsset("Resources/Assets/Effects/" + GetName() + ".effect");
		}

		if(ImGui::CollapsingHeader("Shared Particle Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("Camera Dither");
			bool changed = false;
			changed |= ImGui::Checkbox("Enable##fxCameraDither", &cameraDitherEnabled_);
			ImGui::BeginDisabled(!cameraDitherEnabled_);
			changed |= ImGui::DragFloat("Near##fxCameraDither", &cameraDitherNear_, 0.01f, 0.0f, 1000.0f);
			changed |= ImGui::DragFloat("Far##fxCameraDither", &cameraDitherFar_, 0.01f, 0.0f, 1000.0f);
			ImGui::EndDisabled();
			if(cameraDitherFar_ < cameraDitherNear_) cameraDitherFar_ = cameraDitherNear_;

			ImGui::SameLine();
			if(ImGui::Button("Apply To All Emitters") || changed) {
				for(auto& wp : emitters_) {
					if(auto sp = wp.lock()) {
						sp->GetEmitter()->SetCameraFadeEnabled(cameraDitherEnabled_);
						sp->GetEmitter()->SetCameraFade(cameraDitherNear_, cameraDitherFar_);
						sp->GetEmitter()->SetCameraFadeEnabled(cameraDitherEnabled_);
					}
				}
			}
		}

		ImGui::SeparatorText("Emitters");
		if(ImGui::Button("Add CPU Emitter")) {
			EffectEmitterNodeConfig node{};
			node.name		  = "Emitter";
			node.isDrawEnable = true;
			node.isGpu		  = false;
			AddEmitterNode(node);
		}
		ImGui::SameLine();
		if(ImGui::Button("Add GPU Emitter")) {
			EffectEmitterNodeConfig node{};
			node.name		  = "GpuEmitter";
			node.isDrawEnable = true;
			node.isGpu		  = true;
			AddEmitterNode(node);
		}

		// タブバー開始
		if(ImGui::BeginTabBar("EmittersTabBar", ImGuiTabBarFlags_Reorderable)) {
			// 右端の [+] ボタン（タブバーの末尾に表示）
			if(ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing)) {
				EffectEmitterNodeConfig node{};
				node.name		  = "Emitter";
				node.isDrawEnable = true;
				node.isGpu		  = false;
				AddEmitterNode(node);
			}

			// 遅延削除用
			int removeIndex = -1;

			// 各エミッターをタブとして描画
			for(int i = 0; i < static_cast<int>(emitters_.size()); ++i) {
				auto sp = emitters_[i].lock();
				if(!sp) {
					// 期限切れなら消す
					emitters_.erase(emitters_.begin() + i);
					--i;
					continue;
				}

				// タブラベル
				std::string label = sp->GetName();
				label += "###EmitterTab_";
				// GUID
				label += sp->GetGuid().ToString();

				bool open = true;
				if(ImGui::BeginTabItem(label.c_str(), &open)) {
					// ---------- タブ内容 ----------
					ImGui::Text("GUID: %s", sp->GetGuid().ToString().c_str());
					const bool isGpuEmitter = std::dynamic_pointer_cast<GpuFxEmitter>(sp->GetEmitter()) != nullptr;
					ImGui::Text("Type: %s", isGpuEmitter ? "GPU Particle" : "CPU Particle");

					// 名前編集
					{
						std::string editableName = sp->GetName();
						char		buf[128];
						std::snprintf(buf, sizeof(buf), "%s", editableName.c_str());
						ImGui::SetNextItemWidth(240.0f);
						if(ImGui::InputText("Name", buf, sizeof(buf))) {
							sp->SetName(std::string(buf), objectType_); // シーン側の命名規約に合わせて
						}
					}

					sp->GetWorldTransform().ShowImGui("Emitter Transform");
					sp->ShowGui();

					ImGui::EndTabItem();
				}

				// タブの [x] で閉じた場合は削除予約
				if(!open) removeIndex = i;

				// タブのコンテキストメニュー（右クリック）
				if(ImGui::BeginPopupContextItem((std::string("ctx_") + sp->GetGuid().ToString()).c_str())) {
					if(ImGui::MenuItem("Duplicate")) {
						// 簡易複製：Config 抜き出し→ AddEmitterNode
						EffectEmitterNodeConfig node{};
						node.name		= sp->GetName() + "_Copy";
						node.parentGuid = this->GetGuid();
						node.transform  = sp->GetWorldTransform().ExtractConfig();
						sp->GetEmitter()->ExtractConfigTo(node.emitter);
						node.isDrawEnable = true;
						node.isGpu = std::dynamic_pointer_cast<GpuFxEmitter>(sp->GetEmitter()) != nullptr;
						AddEmitterNode(node);
					}
					if(ImGui::MenuItem("Delete")) {
						removeIndex = i;
					}
					ImGui::EndPopup();
				}
			}

			// 予約削除を実行
			if(removeIndex >= 0 && removeIndex < static_cast<int>(emitters_.size())) {
				if(auto sp = emitters_[removeIndex].lock()) {
					RemoveEmitterByGuid(sp->GetGuid());
				} else {
					emitters_.erase(emitters_.begin() + removeIndex);
				}
			}

			ImGui::EndTabBar();
		}
	}

	void FxObject::LoadFromPath(const std::string& path) {
		if(path.ends_with(".effect")) {
			LoadEffectAsset(path);
			return;
		}

		config_.LoadConfig(path);
		ApplyConfig();
	}

	bool FxObject::SaveEffectAsset(const std::string& path) {
		ExtractConfig();
		auto asset = EffectAsset::FromObjectConfig(config_.GetConfig());
		return asset.Save(path);
	}

	bool FxObject::LoadEffectAsset(const std::string& path) {
		EffectAsset asset;
		if(!asset.Load(path)) return false;

		config_.GetConfig() = asset.ToObjectConfig();
		ApplyConfig();
		return true;
	}

	size_t FxObject::GetLiveEmitterCount() const {
		size_t count = 0;
		for(const auto& wp : emitters_) {
			if(wp.lock()) {
				++count;
			}
		}
		return count;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		config 適用
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxObject::ApplyConfig() {
		const auto& cfg = config_.GetConfig();

		// ルート（エフェクト本体）
		name_	  = cfg.name;
		parentId_ = cfg.parentGuid;
		worldTransform_.ApplyConfig(cfg.transform);

		// 子を構築
		RebuildChildrenFromConfig();
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		config 掃き出し
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxObject::ExtractConfig() {
		auto& cfg = config_.GetConfig();

		// ルートを書き出し
		cfg.name	   = name_;
		cfg.parentGuid = parentId_;
		cfg.transform  = worldTransform_.ExtractConfig();

		// 子から同期
		SyncConfigFromChildren();
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		json 適用
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxObject::ApplyConfigFromJson(const nlohmann::json& j) { config_.ApplyConfigFromJson(j); }

	/////////////////////////////////////////////////////////////////////////////////////////
	//		json 掃き出し
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxObject::ExtractConfigToJson(nlohmann::json& j) const {
		const_cast<FxObject*>(this)->ExtractConfig();
		config_.ExtractConfigToJson(j);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		クラス名取得
	/////////////////////////////////////////////////////////////////////////////////////////
	std::string_view FxObject::GetObjectClassName() const { return "FxObject"; }

	void FxObject::SetWorldPosition(const CalyxEngine::Vector3& pos) {
		worldTransform_.translation = pos;
		SyncChildrenFromWorldTransform();
	}

	void FxObject::SyncChildrenFromWorldTransform() {
		worldTransform_.Update();

		for(auto it = emitters_.begin(); it != emitters_.end();) {
			if(auto sp = it->lock()) {
				sp->SyncEmitterFromWorldTransform();
				++it;
			} else {
				it = emitters_.erase(it);
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		コンフィグからエフェクトの再構築
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxObject::RebuildChildrenFromConfig() {
		++emitterRevision_;

		std::vector<std::shared_ptr<ParticleSystemObject>> oldEmitters;
		oldEmitters.reserve(emitters_.size());
		for(auto& wp : emitters_) {
			if(auto sp = wp.lock()) {
				oldEmitters.push_back(sp);
			}
		}
		emitters_.clear();

		if(auto* ctx = SceneContext::Current()) {
			for(auto& sp : oldEmitters) {
				ctx->RemoveObject(sp);
			}
		} else {
			for(auto& sp : oldEmitters) {
				sp->SetParent(nullptr);
			}
		}

		// Config から再構築
		const auto& cfg = config_.GetConfig();
		for(const auto& n : cfg.emitters) {
			AddEmitterNode(n);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		子供からコンフィグの同期k
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxObject::SyncConfigFromChildren() {
		auto& cfg = config_.GetConfig();
		cfg.emitters.clear();

		for(auto it = emitters_.begin(); it != emitters_.end();) {
			if(auto sp = it->lock()) {
				sp->GetWorldTransform().Update();
				sp->SyncEmitterFromWorldTransform();

				EffectEmitterNodeConfig n{};
				n.name		 = sp->GetName();
				n.guid		 = sp->GetGuid();
				n.parentGuid = this->GetGuid();
				n.transform  = sp->GetWorldTransform().ExtractConfig();

				sp->GetEmitter()->ExtractConfigTo(n.emitter);
				n.isDrawEnable = sp->IsDrawEnable();
				n.isGpu = std::dynamic_pointer_cast<GpuFxEmitter>(sp->GetEmitter()) != nullptr;

				cfg.emitters.push_back(std::move(n));
				++it;
			} else {
				// 期限切れを掃除
				it = emitters_.erase(it);
			}
		}
	}


	/////////////////////////////////////////////////////////////////////////////////////////
	//		エミッター単位
	/////////////////////////////////////////////////////////////////////////////////////////
	std::shared_ptr<ParticleSystemObject>
	FxObject::AddEmitterNode(const EffectEmitterNodeConfig& node) {
		std::shared_ptr<BaseEmitter> emitter;
		if(node.isGpu) {
			auto gpuEmitter = std::make_shared<GpuFxEmitter>();
			gpuEmitter->Initialize();
			emitter = gpuEmitter;
		} else {
			emitter = std::make_shared<FxEmitter>();
		}

		auto child = SceneAPI::Instantiate<ParticleSystemObject>(
			node.name.empty() ? "emitter" : node.name,
			emitter);

		if(node.guid.isValid()) {
			child->SetGuid(node.guid);
		}

		child->GetWorldTransform().ApplyConfig(node.transform);
		child->GetEmitter()->ApplyConfigFrom(node.emitter);
		child->SetDrawEnable(node.isDrawEnable);
		child->SetParent(shared_from_this());

		emitters_.push_back(child);
		++emitterRevision_;
		return child;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		idから削除
	/////////////////////////////////////////////////////////////////////////////////////////
	void FxObject::RemoveEmitterByGuid(const Guid& id) {
		for(auto it = emitters_.begin(); it != emitters_.end();) {
			auto sp = it->lock();
			if(!sp) {
				it = emitters_.erase(it);
				continue;
			}
			if(sp->GetGuid() == id) {
				it = emitters_.erase(it);
				++emitterRevision_;
				if(auto* ctx = SceneContext::Current()) {
					ctx->RemoveObject(sp);
				} else {
					sp->SetParent(nullptr);
				}
			} else {
				++it;
			}
		}
	}

} // namespace CalyxEngine
