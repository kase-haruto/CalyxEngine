#pragma once
namespace Cx {
	namespace Ease {

		enum class EaseType {
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

		// ImGuiで選択UIを表示
		void SelectEase(EaseType& type);
	}
}