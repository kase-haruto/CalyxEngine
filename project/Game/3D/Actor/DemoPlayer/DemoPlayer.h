#pragma once
#include <Engine/Objects/3D/Actor/Actor.h>

/*-----------------------------------------------------------------------------------------
 * DemoPlayer
 * - 本エンジンのデモ用操作可能キャラクター
 * - CALYX_OBJECTを使用してエディタ上で配置可能にする
 *---------------------------------------------------------------------------------------*/
CALYX_OBJECT(Category = Actor, DisplayName = "Demo Player", Icon = "UI/Tool/cube.dds")
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

	void Update(float dt) override;

private:
	//===================================================================*/
	//						private methods
	//===================================================================*/


};
