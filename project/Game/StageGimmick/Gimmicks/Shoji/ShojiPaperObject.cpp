#include "ShojiPaperObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(ShojiPaperObject)

ShojiPaperObject::ShojiPaperObject(
	const std::string& modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void ShojiPaperObject::ObjectInitialize() {
	
	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}
}

void ShojiPaperObject::ObjectUpdate(float) {
}
