#pragma once

/* ========================================================================
/*	include space
/* ===================================================================== */
// game
#include "IBossAttack.h"


// c++
#include <cstdint>

/*-----------------------------------------------------------------------------------------
 * BossHomingSpreadShoot
 * - ホーミングスプレー射撃攻撃クラス
 * - 複数のホーミング弾を広範囲に撒き散らす攻撃を実行する
 * - 弾は一定時間直進した後、ターゲットに向かってホーミングを開始する
 *---------------------------------------------------------------------------------------*/
class BossHomingSpreadShoot final
	: public IBossAttack {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	BossHomingSpreadShoot();
	~BossHomingSpreadShoot() override;

	bool Execute(class Boss& boss,class BossShootingController& shooter) const override;
	void ShowGui() override;

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/

	struct BossHomigSqreadShootParam :
		public IBossAttackParam {

		// 登録
		BossHomigSqreadShootParam();

		// 調整可能パラメータ
		int32_t bulletCount   = 4;    //< 撒く弾の数
		float   initialSpeed  = 10.0f; //< 最初の直進スピード
		float   homingDelay   = 0.8f; //< 追尾開始までの遅延
		float   startAngleDeg = 0.0f; //< 撒き始めの角度
	} param_;
};