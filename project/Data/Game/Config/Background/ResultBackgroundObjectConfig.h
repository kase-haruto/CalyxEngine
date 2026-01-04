#pragma once

#include <Engine/Foundation/Serialization/SerializableObject.h>
#include "../hud/HudTransformMotionConfig.h"

class ResultBackgroundObjectConfig final
	: public CalyxEngine::SerializableObject{
public:
	ResultBackgroundObjectConfig();
	CalyxEngine::ParamPath GetParamPath() const override;

	void ShowGui();

	// 背景オブジェクトのテクスチャパス
	float uvRotate;
	float uvScrollSpeed;

	CalyxMath::Vector2 topPosition;
	CalyxMath::Vector2 bottomPosition;

	float topRotate;
	float bottomRotate;
};