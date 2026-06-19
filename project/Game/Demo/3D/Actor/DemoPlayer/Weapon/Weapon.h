#pragma once
#include <Engine\Objects\3D\Actor\BaseGameObject.h>

/*-----------------------------------------------------------------------------------------
 * Weapon
 * - playerの所有する武器
 *---------------------------------------------------------------------------------------*/
CALYX_OBJECT(Category = GameObject, DisplayName = "Weapon")
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
