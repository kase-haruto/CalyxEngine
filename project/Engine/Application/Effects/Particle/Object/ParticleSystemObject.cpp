#include "ParticleSystemObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/System/Event/EventBus.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Assets/System/AssetDragPayload.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
ParticleSystemObject::ParticleSystemObject() =default;
ParticleSystemObject::ParticleSystemObject(const std::string& name) {
	SceneObject::SetName(name,ObjectType::Effect);
	// デフォルト値の設定
	velocity_.SetConstant({0.0f, 2.0f, 0.0f});
	lifetime_.SetConstant({1.0f});
	scale_.SetConstant({1.0f, 1.0f, 1.0f});
}
ParticleSystemObject::~ParticleSystemObject() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		常時更新
/////////////////////////////////////////////////////////////////////////////////////////
void ParticleSystemObject::AlwaysUpdate([[maybe_unused]] float dt) {
	if(FxEmitter::IsPlaying()) {
		worldTransform_.Update();
		position_ = worldTransform_.GetWorldPosition();
	}

	FxEmitter::Update(dt);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		debug gui
/////////////////////////////////////////////////////////////////////////////////////////
void ParticleSystemObject::ShowGui() {
	config_.ShowGui();

	ImGui::PushID(this);

	// =============================
	// マテリアルセクション
	// =============================
	ImGui::SeparatorText("Material");
	ImGui::ColorEdit4("Color", &material_.color.x);

	// 現在のパスを表示
	// ---- ドラッグ&ドロップでテクスチャ適用 ----
	ImGui::Text("Texture (Drag & Drop from Assets)");
	// ドロップ領域（InvisibleButton で有効アイテム化）
	ImVec2 dropSize(ImGui::GetContentRegionAvail().x, 56.0f);
	ImGui::InvisibleButton("##TextureDrop", dropSize);

	// 見た目（枠とテキスト）
	const bool hovered = ImGui::IsItemHovered();
	const ImVec2 rmin = ImGui::GetItemRectMin();
	const ImVec2 rmax = ImGui::GetItemRectMax();
	ImGui::GetWindowDrawList()->AddRect(
		rmin, rmax, hovered ? IM_COL32(120, 180, 255, 220) : IM_COL32(90, 90, 90, 160),
		8.0f, 0, 2.0f);
	ImGui::GetWindowDrawList()->AddText(
		ImVec2(rmin.x + 8.0f, rmin.y + 8.0f),
		IM_COL32(230, 230, 230, 255),
		"Drop a Texture here");

	// 受け取り
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("CALYX_ASSET")) {
			const AssetDragPayload payload =
				*reinterpret_cast<const AssetDragPayload*>(p->Data);
			if (payload.type == AssetType::Texture) {
				if (LoadTextureByGuid(payload.guid)) {
					// コンフィグ（保存用）にも反映
					config_.GetConfig().textureGuid = payload.guid;
				} else {
					ImGui::OpenPopup("TextureDropError");
				}
			}
		}
		ImGui::EndDragDropTarget();
	}

	// 失敗メッセージ（2D 以外の SRV 等）
	if (ImGui::BeginPopup("TextureDropError")) {
		ImGui::TextUnformatted("このテクスチャは適用できません（2D以外/未対応形式）。");
		ImGui::EndPopup();
	}

	// 現在のテクスチャ表示（GUID→ファイル名）
	auto labelFromGuid = [](const Guid& g)->std::string {
		if (!g.isValid()) return "(none)";
		auto* db = AssetDatabase::GetInstance();
		for (auto* r : db->GetView()) {
			if (r && r->type == AssetType::Texture && r->guid == g) {
				return r->sourcePath.filename().string();
			}
		}
		return "(missing)";
	};
	// =============================
	// Emit設定
	// =============================
	ImGui::SeparatorText("Emit");
	ImGui::Text("emitCount: %d", units_.size());
	GuiCmd::DragFloat3("position", position_);
	GuiCmd::DragFloat("emitRate", emitRate_, 0.01f, 0.0f, 10.0f);

	GuiCmd::CheckBox("isComplement", isComplement_);
	GuiCmd::CheckBox("isStatic", isStatic_);

	ImGuiHelpers::DrawFxParamGui("Scale", scale_);
	ImGuiHelpers::DrawFxParamGui("Velocity", velocity_);
	ImGuiHelpers::DrawFxParamGui("Lifetime", lifetime_);

	// =============================
	// 再生制御
	// =============================
	ImGui::Spacing();
	ImGui::SeparatorText("Emitter Controls");
	if (ImGui::Button("Play")) { Play(); }
	ImGui::SameLine();
	if (ImGui::Button("Stop")) { Stop(); }
	ImGui::SameLine();
	if (ImGui::Button("Reset")) { Reset(); }

	// =============================
	// OneShot
	// =============================
	ImGui::Spacing();
	ImGui::SeparatorText("OneShot Settings");
	GuiCmd::CheckBox("OneShot", isOneShot_);
	if (isOneShot_) {
		ImGui::DragInt("Emit Count", &emitCount_, 1, 1, kMaxUnits_);
		GuiCmd::CheckBox("Auto Destroy", autoDestroy_);
		GuiCmd::DragFloat("Emit Delay", emitDelay_, 0.01f, 0.0f, 10.0f);
	} else {
		GuiCmd::DragFloat("Emit Duration", emitDuration_, 0.01f, -1.0f, 60.0f);
	}

	// =============================
	// モジュール
	// =============================
	if (moduleContainer_) {
		moduleContainer_->ShowModulesGui();
		moduleContainer_->ShowAvailableModulesGui();
	}

	ImGui::PopID();
}


void ParticleSystemObject::SetDrawEnable(bool isDrawEnable) {
	FxEmitter::SetDrawEnable(isDrawEnable);

	// 子にも適用
	for(const auto& child : children_) { if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(child)) { ps->SetDrawEnable(isDrawEnable); } }
}

void ParticleSystemObject::ApplyConfig() {
	const auto& cfg = config_.GetConfig();

	// FxEmitter 設定反映
	FxEmitter::ApplyConfigFrom(cfg); // config_ は ParticleSystemObjectConfig のはず

	textureHandle_ = TextureManager::GetInstance()->LoadTexture("Textures/"+GetTexturePath());

	// SceneObject 情報
	name_     = cfg.name;
	id_       = cfg.guid;
	parentId_ = cfg.parentGuid;

	worldTransform_.ApplyConfig(cfg.transform);
}

void ParticleSystemObject::ExtractConfig() {
	auto& cfg = config_.GetConfig();
	FxEmitter::ExtractConfigTo(cfg); // config_ は ParticleSystemObjectConfig

	cfg.name       = name_;
	cfg.guid       = id_;
	cfg.parentGuid = parentId_;
	worldTransform_.ExtractConfig();
}

void ParticleSystemObject::ApplyConfigFromJson(const nlohmann::json& j) {
	config_.ApplyConfigFromJson(j);
	ApplyConfig();
}

void ParticleSystemObject::ExtractConfigToJson(nlohmann::json& j) const {
	const_cast<ParticleSystemObject*>(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);
}

void ParticleSystemObject::LoadConfig(const std::string& path) {
	config_.LoadConfig(path);
	ApplyConfig();
}

void ParticleSystemObject::SaveConfig(const std::string& path) const {
	const_cast<ParticleSystemObject*>(this)->ExtractConfig();
	config_.SaveConfig(path);
}

void ParticleSystemObject::PlayRecursive() {
	Play();
	for(const auto& child : children_) { if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(child)) { ps->PlayRecursive(); } }
}

void ParticleSystemObject::StopRecursive() {
	Stop();
	for(const auto& child : children_) { if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(child)) { ps->StopRecursive(); } }
}

void ParticleSystemObject::ResetRecursive() {
	Reset();
	for(const auto& child : children_) { if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(child)) { ps->ResetRecursive(); } }
}


bool ParticleSystemObject::LoadTextureByGuid(const Guid& g) {
	if (!g.isValid()) return false;

	auto h = TextureManager::GetInstance()->LoadTexture(g);
	if (!h.ptr) return false;

	textureHandle_ = h;
	textureGuid_ = g;
	return true;
}

REGISTER_SCENE_OBJECT(ParticleSystemObject)