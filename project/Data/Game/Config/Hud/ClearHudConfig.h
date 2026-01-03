#pragma once
#include "Engine/Foundation/Serialization/SerializableObject.h"
#include "Engine/Foundation/Utility/Ease/CxEase.h"

/* ----------------------------------------------------
 *	ClearLogoHudConfig class
 *	- クリアロゴHUD設定クラス
 * ---------------------------------------------------*/
#pragma once
#include "HudTransformMotionConfig.h"

class ClearLogoHudConfig final
	: public Calyx2D::HudTransformMotionConfig {
public:
	ClearLogoHudConfig();
	CalyxEngine::ParamPath GetParamPath() const override;
};