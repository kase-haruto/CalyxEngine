#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */
#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/Event/BaseEventObject.h>

// fwd
class Camera3d;

/* ========================================================================
/*		カメラのアクションを行うイベント
/* ===================================================================== */
CALYX_OBJECT(Category = Event, DisplayName = "Camera Event")
class CameraEventObject :
	public BaseEventObject {
public:
	//===================================================================*/
	//					 public methods
	//===================================================================*/
	CameraEventObject();
	CameraEventObject(const std::string& name);
	~CameraEventObject()override;

	
	std::string GetObjectTypeName() const override { return name_; }

protected:
	//===================================================================*/
	//					 protected methods
	//===================================================================*/
	Camera3d* cam_ = nullptr;		//< eventで操作対象のカメラ
};
