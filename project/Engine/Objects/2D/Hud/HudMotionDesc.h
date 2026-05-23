#pragma once
#include "../../../Foundation/Utility/Ease/CxEase.h"

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * HUDモーション記述構造体
	 * - HUDのモーション開始・終了値、時間、イージングをまとめた構造体
	 *---------------------------------------------------------------------------------------*/
	template<typename T>
	struct HudMotionDesc {
		T start{};
		T end{};
		float duration = 0.5f;
		CalyxEngine::EaseType easing = CalyxEngine::EaseType::Linear;
	};

}