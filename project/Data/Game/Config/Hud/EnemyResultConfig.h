#pragma once

/*====================================================================
 *	include space
 *====================================================================*/
#include "HudTransformMotionConfig.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"
#include "Engine/Foundation/Utility/Ease/CxEase.h"
#include "Engine/Foundation/Math/Vector2.h"

// c++
#include <string>

/* ----------------------------------------------------
 *	EnemyResultConfig class
 *	- 敵撃破数リザルトHUD設定クラス
 * ---------------------------------------------------*/
class EnemyResultConfig final
	: public Calyx2D::HudTransformMotionConfig {
public:
	EnemyResultConfig();
	CalyxEngine::ParamPath GetParamPath() const override;

	//====================================================
	// 共通
	//====================================================
	float   delaySec     = 0.5f; //< 最初の行の遅延
	float   countUpSec   = 0.6f; //< 1行あたりのカウント時間
	int32_t countEaseInt =
		static_cast<int32_t>(CalyxEase::EaseType::EaseOutCubic);

	//====================================================
	// EnemyResult
	//====================================================
	float rowSpacing     = 56.0f; //< 行間
	float perRowDelaySec = 0.15f; //< 行ごとの追加ディレイ

	// レイアウト（アンカー基準）
	CalyxMath::Vector2 iconOffset   = {-80.0f,0.0f};
	CalyxMath::Vector2 numberOffset = {80.0f,0.0f};

	// サイズ
	CalyxMath::Vector2 iconSize  = {64.0f,64.0f};
	CalyxMath::Vector2 digitSize = {32.0f,32.0f};
};