#include "FxObject.h"
/* ========================================================================
/*		include space
/* ===================================================================== */
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Utility/SceneUtility.h>

REGISTER_SCENE_OBJECT(FxObject);

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
FxObject::FxObject(const std::string& name) { SceneObject::SetName(name,ObjectType::Effect); }
FxObject::~FxObject() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void FxObject::Initialize() {
	// 調節パラメータ適用

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
	worldTransform_.Update();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		再生
/////////////////////////////////////////////////////////////////////////////////////////
void FxObject::PlayAll() const {
	for(const auto& particle : emitters_) {
		// effect再生
		particle->Play();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		停止
/////////////////////////////////////////////////////////////////////////////////////////
void FxObject::StopAll() const {
	for(const auto& particle : emitters_) {
		// effect停止
		particle->Play();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		再再生
/////////////////////////////////////////////////////////////////////////////////////////
void FxObject::RestartAll() const {
	for(const auto& particle : emitters_) {
		// effect再々再生
		particle->Reset();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デバッグui
/////////////////////////////////////////////////////////////////////////////////////////
void FxObject::ShowGui() {
	// ルート Transform
	worldTransform_.ShowImGui();

	// 一括操作
	if(ImGui::Button("Play All")) PlayAll();
	ImGui::SameLine();
	if(ImGui::Button("Stop All")) StopAll();
	ImGui::SameLine();
	if(ImGui::Button("Reset All")) RestartAll();

	ImGui::SeparatorText("Emitters");

	// タブバー開始
	if(ImGui::BeginTabBar("EmittersTabBar",ImGuiTabBarFlags_Reorderable)) {
		// 右端の [+] ボタン（タブバーの末尾に表示）
		if(ImGui::TabItemButton("+",ImGuiTabItemFlags_Trailing)) {
			EffectEmitterNodeConfig node{};
			node.name         = "Emitter";
			node.isDrawEnable = true;
			AddEmitterNode(node);
		}

		// 遅延削除用
		int removeIndex = -1;

		// 各エミッターをタブとして描画
		for(int i = 0; i < (int)emitters_.size(); ++i) {
			auto& sp = emitters_[i];
			if(!sp) continue;

			// タブラベル（
			std::string label = sp->GetName();
			label += "###EmitterTab_";
			// GUID
			label += sp->GetGuid().ToString();

			bool open = true;
			if(ImGui::BeginTabItem(label.c_str(),&open)) {
				// ---------- タブ内容 ----------
				ImGui::Text("GUID: %s",sp->GetGuid().ToString().c_str());

				// 名前編集
				{
					std::string editableName = sp->GetName();
					char        buf[128];
					std::snprintf(buf,sizeof(buf),"%s",editableName.c_str());
					ImGui::SetNextItemWidth(240.0f);
					if(ImGui::InputText("Name",buf,sizeof(buf))) {
						sp->SetName(std::string(buf),objectType_); // シーン側の命名規約に合わせて
					}
				}

				// ドロー可否
				{
					bool draw = true; // 必要なら PSO にゲッターを追加して取得
					if(ImGui::Checkbox("Draw Enable",&draw)) { sp->SetDrawEnable(draw); }
				}

				// プレイ系
				if(ImGui::Button("Play")) { sp->Play(); }
				ImGui::SameLine();
				if(ImGui::Button("Stop")) { sp->Stop(); }
				ImGui::SameLine();
				if(ImGui::Button("Reset")) { sp->Reset(); }

				// 並べ替え（保存順に反映したい場合）
				{
					if(i > 0) {
						ImGui::SameLine();
						if(ImGui::Button("Move Up")) { std::swap(emitters_[i - 1],emitters_[i]); }
					}
					if(i + 1 < (int)emitters_.size()) {
						ImGui::SameLine();
						if(ImGui::Button("Move Down")) { std::swap(emitters_[i],emitters_[i + 1]); }
					}
				}

				ImGui::Separator();

				// Transform（子のローカル or ワールド、あなたの編集方針に合わせて）
				if(ImGui::CollapsingHeader("Transform",ImGuiTreeNodeFlags_DefaultOpen)) {
					sp->GetWorldTransform().ShowImGui(); // ローカル編集なら専用GUIに置換
				}

				// エミッター詳細GUI（既存を委譲）
				if(ImGui::CollapsingHeader("Emitter Params",ImGuiTreeNodeFlags_DefaultOpen)) {
					sp->ShowGui(); // 既存の FxEmitter::ShowGui() が呼ばれる
				}

				ImGui::EndTabItem();
			}

			// タブの [x] で閉じた場合は削除予約
			if(!open) removeIndex = i;

			// タブのコンテキストメニュー（右クリック）
			if(ImGui::BeginPopupContextItem((std::string("ctx_") + sp->GetGuid().ToString()).c_str())) {
				if(ImGui::MenuItem("Duplicate")) {
					// 簡易複製：Config 抜き出し→ AddEmitterNode
					EffectEmitterNodeConfig node{};
					node.name       = sp->GetName() + "_Copy";
					node.parentGuid = this->GetGuid();
					sp->GetWorldTransform().ExtractConfig();
					node.transform = sp->GetConfigObject().GetConfig().transform;
					sp->FxEmitter::ExtractConfigTo(node.emitter);
					node.isDrawEnable = true;
					AddEmitterNode(node);
				}
				if(ImGui::MenuItem("Delete")) { removeIndex = i; }
				ImGui::EndPopup();
			}
		}

		// 予約削除を実行
		if(removeIndex >= 0 && removeIndex < (int)emitters_.size()) {
			if(emitters_[removeIndex]) emitters_[removeIndex]->SetParent(nullptr);
			emitters_.erase(emitters_.begin() + removeIndex);
		}

		ImGui::EndTabBar();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		config 適用
/////////////////////////////////////////////////////////////////////////////////////////
void FxObject::ApplyConfig() {
	const auto& cfg = config_.GetConfig();

	// ルート（エフェクト本体）
	name_     = cfg.name;
	id_       = cfg.guid;
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
	cfg.name       = name_;
	cfg.guid       = id_;
	cfg.parentGuid = parentId_;
	worldTransform_.ExtractConfig();

	// 子から同期
	SyncConfigFromChildren();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		json 適用
/////////////////////////////////////////////////////////////////////////////////////////
void FxObject::ApplyConfigFromJson(const nlohmann::json& j) {
	config_.ApplyConfigFromJson(j);
	ApplyConfig();
}

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
std::string_view FxObject::GetTypeName() const { return "FxObject"; }

/////////////////////////////////////////////////////////////////////////////////////////
//		コンフィグからエフェクトの再構築
/////////////////////////////////////////////////////////////////////////////////////////
void FxObject::RebuildChildrenFromConfig() {
	// 既存の子  を全削除
	for(auto& sp : emitters_) if(sp) this->SetParent(nullptr);
	emitters_.clear();

	// Config から生成
	const auto& cfg = config_.GetConfig();
	for(const auto& n : cfg.emitters) { AddEmitterNode(n); }
}

/////////////////////////////////////////////////////////////////////////////////////////
//		子供からコンフィグの同期k
/////////////////////////////////////////////////////////////////////////////////////////
void FxObject::SyncConfigFromChildren() {
	auto& cfg = config_.GetConfig();
	cfg.emitters.clear();

	for(auto& sp : emitters_) {
		if(!sp) continue;
		EffectEmitterNodeConfig n{};
		n.name       = sp->GetName();
		n.guid       = sp->GetGuid();
		n.parentGuid = this->GetGuid();

		sp->GetWorldTransform().ExtractConfig();
		n.transform = sp->GetConfigObject().GetConfig().transform;

		sp->FxEmitter::ExtractConfigTo(n.emitter);
		cfg.emitters.push_back(std::move(n));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		エミッター単位
/////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<ParticleSystemObject>
FxObject::AddEmitterNode(const EffectEmitterNodeConfig& node) {
	// 子を生成
	auto child = SceneAPI::Instantiate<ParticleSystemObject>("emitter");
	child->SetDrawEnable(node.isDrawEnable);

	// 親子付け（Transform 親子・Scene 階層）
	this->AddChild(child);

	// Transform と Emitter を適用
	child->GetWorldTransform().ApplyConfig(node.transform);
	child->FxEmitter::ApplyConfigFrom(node.emitter);

	emitters_.push_back(child);
	return child;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		idから削除
/////////////////////////////////////////////////////////////////////////////////////////
void FxObject::RemoveEmitterByGuid(const Guid& id) {
	auto it = std::find_if(emitters_.begin(),emitters_.end(),
						   [&](const std::shared_ptr<ParticleSystemObject>& sp) { return sp && sp->GetGuid() == id; });
	if(it != emitters_.end()) {
		this->SetParent(nullptr);
		emitters_.erase(it);
	}
}