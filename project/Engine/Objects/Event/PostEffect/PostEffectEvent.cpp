#include "PostEffectEvent.h"

#include <CalyxEngine/Project.h>
#include <Engine/Application/UI/Panels/AssetPanel.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/PostProcess/Manager/PostEffectManager.h>
#include <filesystem>
#include <externals/imgui/imgui.h>

REGISTER_SCENE_OBJECT(PostEffectEvent)

PostEffectEvent::PostEffectEvent() {
	SetName("PostEffectEvent", ObjectType::Event);
}

void PostEffectEvent::Initialize() {
	// Base Event初期化後にPresetを適用し、PostEffectManagerが利用可能な順序を保証する。
	BaseEventObject::Initialize();
	initialized_ = true;
	ApplyPreset();
}

bool PostEffectEvent::SetPresetAsset(const Guid& guid) {
	// PostEffect種別のAssetだけを受理し、別種別GUIDによる読込失敗を防ぐ。
	const AssetRecord* record = AssetDatabase::GetInstance()->Get(guid);
	if(!record || record->type != AssetType::PostEffect) return false;

	presetGuid_ = guid;
	presetPath_ = record->sourcePath.generic_string();
	return !initialized_ || ApplyPreset();
}

bool PostEffectEvent::ApplyPreset() {
	if(presetGuid_.isValid()) {
		// GUIDから最新Pathを再解決し、Asset移動後も保存済みEventから追従できるようにする。
		if(const AssetRecord* record = AssetDatabase::GetInstance()->Get(presetGuid_)) {
			presetPath_ = record->sourcePath.generic_string();
		}
	}
	// GUID未解決時は旧Sceneに保存されたPathを互換Fallbackとして利用する。
	if(presetPath_.empty()) return false;
	return PostEffectManager::Get()->LoadPreset(
		Calyx::ResolveAssetPath(presetPath_).generic_string());
}

void PostEffectEvent::DerivativeGui() {
	BaseEventObject::DerivativeGui();
	ImGui::SeparatorText("Post Effect Preset");

	Guid dropped = presetGuid_;
	if(CalyxEngine::AssetPanel::DrawAssetDropTarget(AssetType::PostEffect, &dropped)) {
		SetPresetAsset(dropped);
	}
	ImGui::TextWrapped("%s", presetPath_.c_str());
	if(ImGui::Button("Apply Post Effect Preset")) ApplyPreset();
}

void PostEffectEvent::ApplyDerivedConfigFromJson(
	const nlohmann::json& root, const nlohmann::json* derived) {
	(void)root;
	if(!derived) return;
	// 初期化前のDeserializeでは値だけ保持し、Manager準備後にInitializeから適用する。
	if(derived->contains("presetGuid")) presetGuid_ = derived->at("presetGuid").get<Guid>();
	presetPath_ = derived->value("presetPath", presetPath_);
	if(initialized_) ApplyPreset();
}

void PostEffectEvent::ExtractDerivedConfigToJson(
	nlohmann::json& root, nlohmann::json& derived) const {
	(void)root;
	derived["presetGuid"] = presetGuid_;
	derived["presetPath"] = presetPath_;
}
