#include "PostEffectEvent.h"

#include <Engine/Application/UI/Panels/AssetPanel.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/PostProcess/Manager/PostEffectManager.h>
#include <externals/imgui/imgui.h>

#include <filesystem>

REGISTER_SCENE_OBJECT(PostEffectEvent)

void PostEffectEvent::Initialize() {
	BaseEventObject::Initialize();
	initialized_ = true;
	ApplyPreset();
}

bool PostEffectEvent::SetPresetAsset(const Guid& guid) {
	const AssetRecord* record = AssetDatabase::GetInstance()->Get(guid);
	if(!record || record->type != AssetType::PostEffect) return false;

	presetGuid_ = guid;
	presetPath_ = record->sourcePath.generic_string();
	return !initialized_ || ApplyPreset();
}

bool PostEffectEvent::ApplyPreset() {
	if(presetGuid_.isValid()) {
		if(const AssetRecord* record = AssetDatabase::GetInstance()->Get(presetGuid_)) {
			presetPath_ = record->sourcePath.generic_string();
		}
	}
	return PostEffectManager::Get()->LoadPreset(presetPath_);
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
	if(derived->contains("presetGuid")) presetGuid_ = derived->at("presetGuid").get<Guid>();
	presetPath_ = derived->value("presetPath", presetPath_);
}

void PostEffectEvent::ExtractDerivedConfigToJson(
	nlohmann::json& root, nlohmann::json& derived) const {
	(void)root;
	derived["presetGuid"] = presetGuid_;
	derived["presetPath"] = presetPath_;
}
