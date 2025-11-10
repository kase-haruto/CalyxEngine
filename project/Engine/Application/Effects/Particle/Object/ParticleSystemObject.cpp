#include "ParticleSystemObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/System/Event/EventBus.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Assets/System/AssetDragPayload.h>
namespace {
void VSeparator(float height = 0.0f, float thickness = 1.0f, float pad = 6.0f) {
	ImVec2 pos  = ImGui::GetCursorScreenPos();
	if (height <= 0.0f) height = ImGui::GetTextLineHeightWithSpacing();

	// 線の色は ImGuiCol_Separator を流用
	ImU32 col = ImGui::GetColorU32(ImGuiCol_Separator);
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float x = pos.x + pad * 0.5f; // ちょい内側に
	dl->AddLine(ImVec2(x, pos.y), ImVec2(x, pos.y + height), col, thickness);

	// レイアウトを前へ送る（幅 = pad + thickness）
	ImGui::Dummy(ImVec2(pad + thickness, height));
	ImGui::SameLine();
}
};

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
ParticleSystemObject::ParticleSystemObject() {
	SceneObject::SetName("ParticleSystemObject", ObjectType::Effect);
}
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
	ImGui::PushID(this);

	// ---- 上部ミニバー：よく触る項目をサッと ----
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Quick Controls");
		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		ImGui::TextUnformatted("Rate");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120);
		GuiCmd::DragFloat("##rate_top", emitRate_, 0.01f, 0.0f, 10.0f);
		ImGui::SameLine();

		ImGui::TextUnformatted("OneShot");
		ImGui::SameLine();
		GuiCmd::CheckBox("##oneshot_top", isOneShot_);

	// ================= Material =================
	if(FxGui::GridScope sec{"Material"}; sec.open) {
		// Color
		FxGui::RowLabel("Color");
		ImGui::ColorEdit4("##color", &material_.color.x);

		// Texture (path表示 + 選択ボタン)
		FxGui::RowLabel("Texture");
		ImGui::BeginGroup();
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
		ImGui::EndGroup();
	}

	// // ================= Billboard =================
	if(FxGui::GridScope sec{"Billboard"}; sec.open) {
		FxGui::RowLabel("Mode");
		static const char* modes[] = {"None", "Full", "AxisY"};
		int current = static_cast<int>(billboardMode_);
		if(ImGui::Combo("##billmode", &current, modes, IM_ARRAYSIZE(modes))) {
			billboardMode_ = static_cast<BillboardMode>(current);
			billboardParams_.mode = current;
			billboardCB_.TransferData(billboardParams_);
		}
	}

	// ================= Emission =================
	if(FxGui::GridScope sec{"Emission"}; sec.open) {
		FxGui::RowLabel("Alive Count");
		ImGui::Text("%zu", units_.size());

		FxGui::RowLabel("World Position");
		GuiCmd::DragFloat3("##pos", position_);

		FxGui::RowLabel("Emit Rate (sec)");
		GuiCmd::DragFloat("##rate", emitRate_, 0.01f, 0.0f, 10.0f);

		FxGui::RowLabel("Complement Trail");
		GuiCmd::CheckBox("##comp", isComplement_);

		FxGui::RowLabel("random Spin on Emit");
		GuiCmd::CheckBox("##randspin", randomSpinEmit_);
	}

	// ================= Params =================
	if(FxGui::GridScope sec{"Params"}; sec.open) {
		FxGui::DrawParam("Scale", scale_);
		FxGui::DrawParam("Velocity", velocity_);
		FxGui::DrawParam("Lifetime", lifetime_);
		FxGui::DrawParam("spin", spin_);
	}

	// ================= Playback =================
	if(FxGui::GridScope sec{"Playback"}; sec.open) {
		FxGui::RowLabel("Controls");
		ImGui::BeginGroup();
		if(ImGui::Button("Play")) {
			Play();
		}
		ImGui::SameLine();
		if(ImGui::Button("Stop")) {
			Stop();
		}
		ImGui::SameLine();
		if(ImGui::Button("Reset")) {
			Reset();
		}
		ImGui::EndGroup();

		FxGui::RowLabel("Draw Enable");
	}

	// ================= One-Shot =================
	if(FxGui::GridScope sec{"One-Shot"}; sec.open) {
		FxGui::RowLabel("Enable");
		if(GuiCmd::CheckBox("##oneshot", isOneShot_)) {
			if(!isOneShot_) {
				hasEmitted_ = false;
			} // OFFに戻した時の自然な継続
		}
		bool tp = GetTimedPreview();
		if (GuiCmd::CheckBox("##timedPrev", tp)) {
			SetTimedPreview(tp);
			// ON にした瞬間に一度流したい場合は以下を有効に
			// if (tp && isOneShot_) RestartOneShot();
		}

		FxGui::RowLabel("Interval (sec)");
		float iv = GetPreviewInterval();
		if (GuiCmd::DragFloat("##prevInt", iv, 0.01f, 0.05f, 10.0f)) {
			SetPreviewInterval(iv);
		}
		
		ImGui::BeginDisabled(!isOneShot_);
		FxGui::RowLabel("Emit Count");
		ImGui::DragInt("##count", &emitCount_, 1, 1, kMaxUnits_);

		FxGui::RowLabel("Auto Destroy");
		GuiCmd::CheckBox("##autoDestroy", autoDestroy_);

		FxGui::RowLabel("Delay (sec)");
		GuiCmd::DragFloat("##delay", emitDelay_, 0.01f, 0.0f, 10.0f);
		ImGui::EndDisabled();

		ImGui::BeginDisabled(isOneShot_);
		FxGui::RowLabel("Emit Duration (sec)");
		GuiCmd::DragFloat("##duration", emitDuration_, 0.01f, -1.0f, 60.0f);
		ImGui::EndDisabled();
	}

	// ================= Modules =================
	if (moduleContainer_) {
		if (FxGui::GridScope sec{"Modules"}; sec.open) {
			// ラベル列
			FxGui::RowLabel("Modules");

			ImGui::BeginGroup();
			// 幅を常にその列いっぱいに
			FxGui::FullWidthScope _fullWidth{};
			// 少しだけ余裕を持たせる見た目
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,  ImVec2(6, 6));

			// --- 有効モジュール（パラメータをここで全部縦に描く） ---
			moduleContainer_->ShowModulesGui();

			// --- 追加パレット（同じ列のまま下に表示） ---
			moduleContainer_->ShowAvailableModulesGui();

			ImGui::PopStyleVar(2);
			ImGui::EndGroup();
		}
	}


	ImGui::Spacing();
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
	FxEmitter::ApplyConfigFrom(cfg);

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