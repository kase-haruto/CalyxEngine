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

	/**
	 * \brief シーン保存・復元で使用するクラス名を取得
	 * \return Weaponの型名
	 */
	std::string_view GetObjectClassName() const override { return "Weapon"; }

private:
	//====================================================================*/
	//                    private methods
	//====================================================================*/
};
