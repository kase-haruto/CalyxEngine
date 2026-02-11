#pragma once
// engine
#include "Engine/Foundation/Math/Vector2.h"
#include <Engine/Foundation/Serialization/SerializableObject.h>
// std
#include <string>

struct DangerSenseConfig :
	public CalyxEngine::SerializableObject {
	//=================================================================*/
	//  function
	//=================================================================*/
	DangerSenseConfig();
	CalyxEngine::ParamPath GetParamPath() const override;

	//=================================================================*/
	//  variable
	//=================================================================*/
	float playerInflate    = 0.5f;
	float margin           = 3.0f;
	float maxCheckDistance = 80.0f;
	int   throttleFrames   = 1;
	float graceTime        = 0.2f; // 回避猶予時間

	// UI
	std::string        uiTex  = "Textures/UI/dodgeUI.dds";
	CalyxMath::Vector2 uiSize = {128.0f,64.0f};
};