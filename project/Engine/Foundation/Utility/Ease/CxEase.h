#pragma once
#include <cstdint>

namespace CalyxEase {

	enum class EaseType :int32_t{
		Linear = 0,
		EaseInQuad,
		EaseOutQuad,
		EaseInOutQuad,
		EaseInCubic,
		EaseOutCubic,
		EaseInOutCubic,
		EaseInSine,
		EaseOutSine,
		EaseInOutSine,
		EaseInExpo,
		EaseOutExpo,
		EaseInOutExpo,
		EaseInBack,
		EaseOutBack,
		EaseInOutBack,
		Count
	};

	// イージング適用
	float ApplyEase(EaseType type, float t);
	float EaseLerp(float start, float end, float t, EaseType ease);
	// ImGuiで選択UIを表示
	bool SelectEase(EaseType& type);
	bool SelectEaseInt(const char* label, int32_t& easeInt);
} // namespace CalyxEase