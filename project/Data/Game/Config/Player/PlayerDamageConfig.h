#pragma once

// engine
#include <Engine/Foundation/Serialization/SerializableObject.h>

struct PlayerDamageConfig :
	public CalyxEngine::SerializableObject {
public:
	//=================================================================*/
	//  function
	//=================================================================*/
	PlayerDamageConfig();
	CalyxEngine::ParamPath GetParamPath() const override;

	//=================================================================*/
	//  variable
	//=================================================================*/
	// --- 定数 ---
	float kHitIFrameSec;
	float kBlinkHz;
	float kBlinkInterval = 1.0f / kBlinkHz;
};