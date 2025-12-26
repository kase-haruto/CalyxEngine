#pragma once
#include "BaseCameraAction.h"
#include <Engine/Foundation/Utility/Ease/CxEase.h>
#include <Engine/objects/ConfigurableObject/ConfigurableObject.h>

/* ========================================================================
/*		include space
/* ===================================================================== */

class CameraTurnAroundAction
	: public BaseCameraAction {
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	CameraTurnAroundAction();
	~CameraTurnAroundAction() override;

	void Update(class BaseCamera* cam, float dt) override;
	void Execute() override;
	void ShowGui() override;

	//---------- accessor ------------------------------------------------//
	void SetEase(CalyxEase::EaseType type) { easeType_ = type; }
	void SetTime(float time) { turnTime_ = time; }
	void SetDirection(const CalyxMath::Vector3& dir) { direction_ = dir; }

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	//---------------- parms -------------//
	CalyxEase::EaseType easeType_ = CalyxEase::EaseType::EaseOutSine;
	CalyxMath::Vector3			   direction_;
	float			   turnTime_ = 0.5f;

	//---------------- internal state -------------//
	bool  turning_ = false;
	float elapsed_ = 0.0f;

	CalyxMath::Quaternion startRot_;
	CalyxMath::Quaternion targetRot_;
};