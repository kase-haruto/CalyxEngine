#pragma once
#include "HudTransformMotionConfig.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"
#include "Engine/Foundation/Utility/Ease/CxEase.h"

/* ----------------------------------------------------
 *	ScoreResultConfig class
 *	- スコアリザルトHUD設定クラス
 * ---------------------------------------------------*/

class ScoreResultConfig final
	: public Calyx2D::HudTransformMotionConfig {
public:
	ScoreResultConfig();
	CalyxEngine::ParamPath GetParamPath() const override;

	// スコア固有
	float    delaySec   = 0.5f;
	float    countUpSec = 1.0f;
	int32_t  countEaseInt = static_cast<int32_t>(CalyxEase::EaseType::EaseOutCubic);
};