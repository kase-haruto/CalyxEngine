#pragma once

#include "../Base/BaseBossState.h"

class BossStateDamage final
	: public BaseBossState {
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	BossStateDamage();
	~BossStateDamage() override;

	/**
	 * \brief 状態に入るときの処理
	 */
	void Enter() override;
	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt) override;

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
};
