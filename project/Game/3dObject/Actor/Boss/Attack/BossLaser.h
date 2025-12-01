#pragma once

#include "IBossAttack.h"

/**
 * \brief ボスのレーザー攻撃クラス
 */
class BossLaser final
	: public IBossAttack {
public:
	//===================================================================*/
	//		public method
	//===================================================================*/
	BossLaser();
	~BossLaser() override;

	bool Execute(class Boss& boss, class BossShootingController& shootingController) const override;
	void ShowGui() override;
	
private:
	//===================================================================*/
	//		private method
	//===================================================================*/
};
