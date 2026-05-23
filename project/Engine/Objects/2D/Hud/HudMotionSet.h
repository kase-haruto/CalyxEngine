#pragma once
#include <optional>
#include "HudMotionDesc.h"
#include "../../../Foundation/Math/Vector2.h"
namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * HUDモーションセット構造体
	 * - HUDの各種モーション設定をまとめた構造体
	 *---------------------------------------------------------------------------------------*/
	struct HudMotionSet {
		std::optional<HudMotionDesc<CalyxEngine::Vector2>> position;
		std::optional<HudMotionDesc<CalyxEngine::Vector2>> scale;
		std::optional<HudMotionDesc<float>>              alpha;
		std::optional<HudMotionDesc<float>>              rotation;

		/**
		 * \brief 表示用GUI
		 * \param label 表示名
		 * \return 変更があったらtrue
		 */
		bool ShowGui(const char* label);
	};


}