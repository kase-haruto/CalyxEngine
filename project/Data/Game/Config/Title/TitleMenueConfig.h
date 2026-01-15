#pragma once
#include "Engine/Foundation/Serialization/SerializableObject.h"

/* ----------------------------------------------------
* TitleMenueConfig
* - タイトル画面のメニュー用の設定
* ---------------------------------------------------*/

struct TitleMenueConfig :
	public CalyxEngine::SerializableObject {
public:
	//================================================*/
	// public method
	//================================================*/
	TitleMenueConfig();
	CalyxEngine::ParamPath GetParamPath() const override;

	CalyxMath::Vector2 basePos_{750, 400.0f};
	CalyxMath::Vector2 baseSize_{256.0f, 64.0f};
	float              space_{120.0f};
	float              enlargedScale_ = 1.20f; // 選択時の最大倍率（1.0=等倍, 1.2=20%拡大）
	float              animSpeed_     = 8.0f;  // t 補間速度（大きいほど速い）
	bool               useBackEase_   = true;  // EaseOutBack か EaseOutQuad か
};