#include "CameraEventObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(CameraEventObject)

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
CameraEventObject::CameraEventObject() = default;
CameraEventObject::CameraEventObject(const std::string& name):BaseEventObject(name) {}
CameraEventObject::~CameraEventObject() =default;