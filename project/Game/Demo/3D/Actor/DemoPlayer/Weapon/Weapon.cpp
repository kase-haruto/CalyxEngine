#include "Weapon.h"

Weapon::Weapon():BaseGameObject("PlayerSword.obj") {}
Weapon::Weapon(const std::string& modelName, std::optional<std::string> objectName):BaseGameObject(modelName, objectName) {

}
Weapon::~Weapon() = default;
