#include "StaticModelObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

#include <utility>

namespace {
	const bool kStaticModelObjectAliasesRegistered = [] {
		SceneObjectRegistry::Get().RegisterAlias("BaseGameObject", "StaticModelObject");
		return true;
	}();
}

StaticModelObject::StaticModelObject()
	: BaseGameObject() {}

StaticModelObject::StaticModelObject(const std::string& modelName, std::optional<std::string> objectName)
	: BaseGameObject(modelName, std::move(objectName)) {}

REGISTER_SCENE_OBJECT(StaticModelObject)
