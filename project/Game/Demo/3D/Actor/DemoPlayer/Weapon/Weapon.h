#pragma once
#include <Engine\Objects\3D\Actor\BaseGameObject.h>

/**
 * @brief Weaponの機能を提供するクラスです。
 */
class Weapon
	: public BaseGameObject{
public:
	//====================================================================*/
	//                    public methods
	//====================================================================*/
	Weapon();
	Weapon(const std::string& modelName, std::optional<std::string> objectName = std::nullopt);
	~Weapon()override;

private:
	//====================================================================*/
	//                    private methods
	//====================================================================*/
};
