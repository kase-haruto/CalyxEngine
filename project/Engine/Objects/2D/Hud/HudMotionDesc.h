#pragma once
#include "../../../Foundation/Utility/Ease/CxEase.h"

namespace Calyx2D {

	/*-----------------------------------------------------------------------------------------
	 * HUDモーション記述構造体
	 * - HUDのモーション開始・終了値、時間、イージングをまとめた構造体
	 *---------------------------------------------------------------------------------------*/
	template<typename T>
	struct HudMotionDesc {
		T start{};
		T end{};
		float duration = 0.5f;
		CalyxEase::EaseType easing = CalyxEase::EaseType::Linear;
	};

}