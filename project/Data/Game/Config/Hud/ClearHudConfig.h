/* ----------------------------------------------------
 *	ClearLogoHudConfig class
 *	- クリアロゴHUD設定クラス
 * ---------------------------------------------------*/
#pragma once
#include "HudTransformMotionConfig.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"

class ClearLogoHudConfig final
	: public Calyx2D::HudTransformMotionConfig {
public:
	ClearLogoHudConfig();
	CalyxEngine::ParamPath GetParamPath() const override;

	CalyxMath::Vector2 logoSize = {304.0f, 176.32f}; // ロゴサイズ（px）
	float amplitude = 20.0f;        // 上下の振幅（px）
	float period = 1.5f;           // 往復にかかる時間（秒）
};