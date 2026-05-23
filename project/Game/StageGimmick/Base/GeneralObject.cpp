#include "GeneralObject.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(GeneralObject)

GeneralObject::GeneralObject(const std::string& modelName, std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {

}

void GeneralObject::ObjectInitialize() {
}

void GeneralObject::ObjectUpdate(float) {
}
