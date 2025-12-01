#pragma once

/* ========================================================================
/*	include space
/* ===================================================================== */
// game
#include "IBossAttack.h"

// c++
#include <cstdint>

/**
 * \brief ボスのホーミング拡散弾発射
 */
class BossHomingSpreadShoot final
	: public IBossAttack {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	BossHomingSpreadShoot();
	~BossHomingSpreadShoot() override;

	bool Execute(class Boss& boss, class BossShootingController& shooter) const override;
	void ShowGui() override;

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	int32_t bulletCount_   = 4;	   //< 撒く弾の数
	float	initialSpeed_  = 1.5f; //< 最初の直進スピード
	float	homingDelay_   = 0.8f; //< 追尾開始までの遅延
	float	startAngleDeg_ = 0.f;  //< 撒き始めの角度
};
