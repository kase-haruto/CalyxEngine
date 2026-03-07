#include "BackgroundActor.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(BackgroundActor)


BackgroundActor::BackgroundActor(const std::string& modelName,std::optional<std::string> objectName)
	: BaseGameObject(modelName,objectName) {}

BackgroundActor::BackgroundActor()  = default;
BackgroundActor::~BackgroundActor() = default;