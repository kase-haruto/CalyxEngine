#include "BossAI.h"

/* ========================================================================
/*  include space
/* ===================================================================== */

// game
#include <Game/3dObject/Actor/Boss/Boss.h>
#include <Game/Battle/Shooting/ShootingController/BossShootingController.h>
#include "../Attack/IBossAttack.h"

///////////////////////////////////////////////////////////////////////////////////////////
///  ctor / dtor
///////////////////////////////////////////////////////////////////////////////////////////
BossAI::BossAI(Boss* owner,BossShootingController* shooter) : owner_(owner), shooter_(shooter) {}
BossAI::~BossAI() =default;

///////////////////////////////////////////////////////////////////////////////////////////
///		更新
///////////////////////////////////////////////////////////////////////////////////////////
void BossAI::Update(float dt) {
	if (!owner_ || !shooter_) return;

	cooldownTimer_ -= dt;
	if (cooldownTimer_ > 0.0f) return;

	// 先頭から順に、実行可能な攻撃を探す
	for (auto& atk : attacks_) {
		//実行
		if (atk->Execute(*owner_, *shooter_)) {
			cooldownTimer_ = atk->GetCooldown();
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
///		攻撃行動の追加
///////////////////////////////////////////////////////////////////////////////////////////
void BossAI::AddAttack(std::unique_ptr<IBossAttack> attack) {
	attacks_.push_back(std::move(attack));
}