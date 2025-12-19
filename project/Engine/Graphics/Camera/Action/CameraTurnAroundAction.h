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
	void SetEase(Cx::Ease::EaseType type) { easeType_ = type; }
	void SetTime(float time) { turnTime_ = time; }
	void SetDirection(const CxMath::Vector3& dir) { direction_ = dir; }

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	//---------------- parms -------------//
	Cx::Ease::EaseType easeType_ = Cx::Ease::EaseType::EaseOutSine;
	CxMath::Vector3			   direction_;
	float			   turnTime_ = 0.5f;

	//---------------- internal state -------------//
	bool  turning_ = false;
	float elapsed_ = 0.0f;

	CxMath::Quaternion startRot_;
	CxMath::Quaternion targetRot_;
};