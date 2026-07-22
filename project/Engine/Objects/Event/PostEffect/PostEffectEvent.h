#pragma once

#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/Event/BaseEventObject.h>

#include <string>

/**
 * \brief シーンに配置して、常設ポストエフェクトのプリセットを適用するイベント。
 *
 * プリセット内のAlwaysノードはシーン中に常時適用され、Triggeredノードは
 * 攻撃などからPostEffectAPI::PlayTriggered()が呼ばれたときだけ再生される。
 */
CALYX_PLACEABLE_OBJECT(Category = Event, DisplayName = "Post Effect Event")
class PostEffectEvent : public BaseEventObject {
public:
	PostEffectEvent();
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

	std::string presetPath_ = "PostEffects/Default.postfx";
	bool initialized_ = false;
};
