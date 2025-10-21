#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */
#include <Engine/Objects/Event/BaseEventObject.h>

// fwd
class Camera3d;


class CameraEventObject :
	public BaseEventObject {
public:
	//===================================================================*/
	//					 public methods
	//===================================================================*/
	CameraEventObject();
	~CameraEventObject()override;

protected:
	//===================================================================*/
	//					 protected methods
	//===================================================================*/
	Camera3d* cam_ = nullptr;		//< eventで操作対象のカメラ
};