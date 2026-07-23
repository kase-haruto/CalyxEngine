#include "PostEffectEvent.h"

#include <CalyxEngine/Project.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/PostProcess/Manager/PostEffectManager.h>

#include <array>
#include <algorithm>
#include <filesystem>

#include <externals/imgui/imgui.h>

REGISTER_SCENE_OBJECT(PostEffectEvent)

PostEffectEvent::PostEffectEvent() {
	SetName("PostEffectEvent", ObjectType::Event);
}

void PostEffectEvent::Initialize() {
	BaseEventObject::Initialize();
	initialized_ = true;
	ApplyPreset();
}

void PostEffectEvent::DerivativeGui() {
	std::array<char, 512> pathBuffer{};
	const size_t copyLength = (std::min)(presetPath_.size(), pathBuffer.size() - 1);
	std::copy_n(presetPath_.data(), copyLength, pathBuffer.data());
	if(ImGui::InputText("Post Effect Preset", pathBuffer.data(), pathBuffer.size())) {
		presetPath_ = pathBuffer.data();
	}
	ImGui::TextDisabled("Assetsからの相対パス（例: PostEffects/Default.postfx）");

	if(ImGui::Button("Apply Post Effect") && initialized_) {
		ApplyPreset();
	}
}

void PostEffectEvent::ApplyDerivedConfigFromJson([[maybe_unused]] const nlohmann::json& root,
											 const nlohmann::json* derived) {
	if(!derived) return;
	presetPath_ = derived->value("presetPath", presetPath_);
	if(initialized_) ApplyPreset();
}

void PostEffectEvent::ExtractDerivedConfigToJson([[maybe_unused]] nlohmann::json& root,
											 nlohmann::json& derived) const {
	derived["presetPath"] = presetPath_;
}

bool PostEffectEvent::ApplyPreset() {
	if(presetPath_.empty()) return false;
	const std::filesystem::path path = Calyx::ResolveAssetPath(presetPath_);
	return PostEffectManager::Get()->LoadPreset(path.generic_string());
}
