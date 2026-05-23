#pragma once
#include <string>
#include <memory>
#include <vector>
#include <Engine/Foundation/Utility/Ease/CxEase.h>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>

enum class PostEffectApplyMode {
	Always,
	Triggered
};

struct PostEffectFloatAnimation {
	std::string parameter;
	float from = 0.0f;
	float to = 0.0f;
	bool useCurrentAsFrom = false;
};

struct PostEffectSlot {
	std::string name;
	bool enabled = true;
	IPostEffectPass* pass = nullptr;
	PostEffectApplyMode applyMode = PostEffectApplyMode::Always;
	float duration = 0.25f;
	CalyxEngine::EaseType ease = CalyxEngine::EaseType::EaseOutSine;
	bool autoDisable = true;
	std::vector<PostEffectFloatAnimation> floatAnimations;
};
