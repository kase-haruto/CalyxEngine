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

	/**
	 * \brief シーン保存・復元で使用するクラス名を取得
	 * \return DemoPlayerの型名
	 */
	std::string_view GetObjectClassName() const override { return "DemoPlayer"; }

private:
	//===================================================================*/
	//						private methods
	//===================================================================*/
	PlayerInput input_;
	DemoPlayerMotor motor_;
	std::shared_ptr<Weapon> weapon_;
};
