#pragma once
#include <Engine/Objects/3D/Actor/Actor.h>

// game
#include "DemoPlayerMotor.h"
#include "Weapon/Weapon.h"

/*-----------------------------------------------------------------------------------------
 * DemoPlayer
 * - 本エンジンのデモ用操作可能キャラクター
 * - CALYX_OBJECTを使用してエディタ上で配置可能にする
 *---------------------------------------------------------------------------------------*/
CALYX_OBJECT(Category = GameObject, DisplayName = "Demo Player", Icon = "UI/Tool/cube.dds")
class DemoPlayer
	:public Actor{
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	/**
	 * \brief コンストラクタ
	*/
	DemoPlayer();
	~DemoPlayer() override = default;

	void Initialize() override;
	void Update(float dt) override;

private:
	//===================================================================*/
	//						private methods
	//===================================================================*/
	PlayerInput input_;
	DemoPlayerMotor motor_;
	std::shared_ptr<Weapon> weapon_;
};
