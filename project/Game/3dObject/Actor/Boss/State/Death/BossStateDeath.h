#pragma once
#include "Game/3dObject/Actor/Boss/State/Base/BaseBossState.h"

class BossStateDeath final
	: public BaseBossState {
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	BossStateDeath();
	~BossStateDeath() override;

	/**
	 * \brief 状態に入るときの処理
	 */
	void Enter() override;
	/**
	* \brief GUI表示
	*/
	void ShowGui() override;
};