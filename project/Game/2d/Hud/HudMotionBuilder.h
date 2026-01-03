#pragma once

#include "Data/Game/Config/Hud/HudTransformMotionConfig.h"
#include "Engine/Objects/2D/Hud/HudMotionSet.h"

namespace Calyx2D {



	// Enter/Stay/Exit 等で使う「モーションセット」の中身を構築する
	void BuildMotionSetFromFlatConfig(const HudTransformMotionConfig& cfg, HudMotionSet& out);

	// “Exit用”に start/end を反転させたい場合の簡易関数
	void BuildExitMotionSetFromFlatConfig(const HudTransformMotionConfig& cfg, HudMotionSet& out) ;

} // namespace Calyx2D