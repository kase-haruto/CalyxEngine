#pragma once
#include "Engine/Foundation/Serialization/SerializableObject.h"

/* ----------------------------------------------------
 *	ClearLogoHudConfig class
 *	- クリアロゴHUD設定クラス
 * ---------------------------------------------------*/
class DimFilterConfig
	: public CalyxEngine::SerializableObject {
public:
	DimFilterConfig();
	CalyxEngine::ParamPath GetParamPath() const override;
	void                   SetName(const std::string& newName) { name = newName; }

	float       startAlpha  = 0.0f;
	float       targetAlpha = 0.6f;
	float       duration    = 0.3f;
	std::string name        = "DimFilter";
};