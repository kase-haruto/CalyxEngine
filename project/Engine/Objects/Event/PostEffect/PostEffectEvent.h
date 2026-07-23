#pragma once

#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <Engine/Objects/Event/BaseEventObject.h>

#include <string>

/**
 * Scene object that owns the active post-effect preset.
 * Always nodes run continuously; Triggered nodes are activated through
 * PostEffectManager::PlayTriggeredEffect/PostEffectAPI::PlayTriggered.
 */
CALYX_PLACEABLE_OBJECT(Category = Event, DisplayName = "Post Effect Event")
class PostEffectEvent : public BaseEventObject {
public:
	PostEffectEvent() = default;
	~PostEffectEvent() override = default;

	void Initialize() override;
	void DerivativeGui() override;

	void ApplyDerivedConfigFromJson(const nlohmann::json& root,
									const nlohmann::json* derived) override;
	void ExtractDerivedConfigToJson(nlohmann::json& root,
									nlohmann::json& derived) const override;

	std::string_view GetObjectClassName() const override { return "PostEffectEvent"; }

private:
	bool ApplyPreset();
	bool SetPresetAsset(const Guid& guid);

	Guid presetGuid_{};
	std::string presetPath_ = "PostEffects/Default.postfx";
	bool initialized_ = false;
};
