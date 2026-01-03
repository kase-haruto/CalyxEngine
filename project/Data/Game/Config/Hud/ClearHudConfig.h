#pragma once
#include "Engine/Foundation/Serialization/SerializableObject.h"
#include "Engine/Foundation/Utility/Ease/CxEase.h"

/* ----------------------------------------------------
 *	ClearLogoHudConfig class
 *	- クリアロゴHUD設定クラス
 * ---------------------------------------------------*/
class ClearLogoHudConfig final
	: public CalyxEngine::SerializableObject {
public:
	//===================================================================*/
	//		public methods
	//===================================================================*/
	ClearLogoHudConfig();

	//- accessor --------------------------------------------------------//
	// getter
	CalyxEngine::ParamPath GetParamPath() const override;

public:
	//===================================================================*/
	//		public members
	//===================================================================*/
	CalyxMath::Vector2 startPosition{};
	CalyxMath::Vector2 stayPosition{};
	CalyxMath::Vector2 scale{300.0f,80.0f};
	CalyxEase::EaseType easeType = CalyxEase::EaseType::EaseOutSine;
	float              duration = 0.5f;
	// AddField に渡すための実体を保持するメンバ
	int32_t easeTypeInt;
};