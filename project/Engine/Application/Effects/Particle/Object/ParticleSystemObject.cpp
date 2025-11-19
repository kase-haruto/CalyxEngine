#include "ParticleSystemObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/System/Event/EventBus.h>
#include <Engine/Assets/Texture/TextureManager.h>


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
	emitter_ = std::make_shared<FxEmitter>();
}
ParticleSystemObject::ParticleSystemObject(const std::string& name) {
	SceneObject::SetName(name,ObjectType::Effect);

	// エミッター
	emitter_ = std::make_shared<FxEmitter>();

	// デフォルト値の設定
	emitter_->velocity_.SetConstant({0.0f, 2.0f, 0.0f});
	emitter_->lifetime_.SetConstant({1.0f});
	emitter_->scale_.SetConstant({1.0f, 1.0f, 1.0f});
}
ParticleSystemObject::~ParticleSystemObject() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		常時更新
/////////////////////////////////////////////////////////////////////////////////////////
void ParticleSystemObject::AlwaysUpdate([[maybe_unused]] float dt) {
	if(emitter_->IsPlaying()) {
		worldTransform_.Update();
		emitter_->SetPosition(worldTransform_.GetWorldPosition());
	}

	emitter_->Update(dt);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		debug gui
/////////////////////////////////////////////////////////////////////////////////////////
void ParticleSystemObject::ShowGui() {
	emitter_->ShowGui();
}



void ParticleSystemObject::SetDrawEnable(bool isDrawEnable) {
	emitter_->SetDrawEnable(isDrawEnable);

	// 子にも適用
	for(const auto& child : children_) { if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(child)) { ps->SetDrawEnable(isDrawEnable); } }
}

void ParticleSystemObject::ApplyConfig() {
	const auto& cfg = config_.GetConfig();

	// FxEmitter 設定反映
	emitter_->ApplyConfigFrom(cfg);
	// SceneObject 情報
	name_     = cfg.name;
	id_       = cfg.guid;
	parentId_ = cfg.parentGuid;

	worldTransform_.ApplyConfig(cfg.transform);
}

void ParticleSystemObject::ExtractConfig() {
	auto& cfg = config_.GetConfig();
	emitter_->ExtractConfigTo(cfg); // config_ は ParticleSystemObjectConfig

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
	emitter_->Play();
	for(const auto& child : children_) { if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(child)) { ps->PlayRecursive(); } }
}

void ParticleSystemObject::StopRecursive() {
	emitter_->Stop();
	for(const auto& child : children_) { if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(child)) { ps->StopRecursive(); } }
}

void ParticleSystemObject::ResetRecursive() {
	emitter_->Reset();
	for(const auto& child : children_) { if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(child)) { ps->ResetRecursive(); } }
}

void ParticleSystemObject::Play() {emitter_->Play();}
void ParticleSystemObject::Stop() {emitter_->Stop();}
void ParticleSystemObject::Reset() {emitter_->Reset();}
REGISTER_SCENE_OBJECT(ParticleSystemObject)