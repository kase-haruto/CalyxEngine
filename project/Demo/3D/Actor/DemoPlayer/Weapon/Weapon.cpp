#include "Weapon.h"

#include "Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h"

REGISTER_SCENE_OBJECT(Weapon)

Weapon::Weapon():BaseGameObject("PlayerSword.obj") {}
Weapon::Weapon(const std::string& modelName, std::optional<std::string> objectName):BaseGameObject(modelName, objectName) {

}
Weapon::~Weapon() = default;