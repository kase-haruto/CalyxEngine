#pragma once
#include "BaseCameraAction.h"
#include <Engine/Foundation/Utility/Ease/EaseTypes.h>
#include <Engine/objects/ConfigurableObject/ConfigurableObject.h>

/* ========================================================================
/*		include space
/* ===================================================================== */

class CameraTurnAroundAction
	: public BaseCameraAction{
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

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	//---------------- parms -------------//
	EaseType easeType_ = EaseType::EaseOutSine;

	float turnTime_ = 0.5f;

	//---------------- internal state -------------//
	bool  turning_ = false;
	float elapsed_ = 0.0f;

	Quaternion startRot_;
	Quaternion targetRot_;
};
