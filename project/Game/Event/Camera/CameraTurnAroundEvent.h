#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */

// engine
#include <Engine/Objects/Event/Camera/CameraEventObject.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>

class CameraTurnAroundEvent final :
	public CameraEventObject {
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	CameraTurnAroundEvent();
	~CameraTurnAroundEvent() override;

	// 発火時処理
	void OnCollisionEnter(Collider* other) override;
	void OnCollisionExit(Collider* other) override;

	// debug ui
	void DerivativeGui() override;

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	Vector3  direction_; //< 振り向く方向
	float    time_;      //< 振り向く時間
	Cx::Ease::EaseType easeType_;  //< 使用イージングタイプ
};