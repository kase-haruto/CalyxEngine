#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include "IBossAttack.h"

/**
 * \brief ボスの通常弾発射
 */
class BossNormalShoot final:
	public IBossAttack {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	BossNormalShoot();
	~BossNormalShoot() override;

	/**
	 * \brief 攻撃を実行を試し、実行可能なら攻撃し、trueを返す
	 * \return 実行可能ならtrue、不可能ならfalse
	 */
	bool Execute(class Boss& boss,class BossShootingController& shooter) const override;
	/**
	 * \brief GUI表示
	 */
	void ShowGui() override;
};