#include "BossAI.h"

/* ========================================================================
/*  include space
/* ===================================================================== */

// game
#include <Game/3dObject/Actor/Boss/Boss.h>
#include <Game/Battle/Shooting/ShootingController/BossShootingController.h>

///////////////////////////////////////////////////////////////////////////////////////////
///  ctor / dtor
///////////////////////////////////////////////////////////////////////////////////////////
BossAI::BossAI(Boss* owner) : owner_(owner) {}
BossAI::~BossAI() = default;

///////////////////////////////////////////////////////////////////////////////////////////
///		更新
///////////////////////////////////////////////////////////////////////////////////////////
void BossAI::Update(float dt) {
	if(!owner_) return;

	cooldownTimer_ -= dt;
	if(cooldownTimer_ > 0.0f) return;
}
std::optional<BossAttackType> BossAI::DecideAttack(float dt) {
	if(!owner_) return std::nullopt;

	// クールダウン
	cooldownTimer_ -= dt;
	if(cooldownTimer_ > 0.f)
		return std::nullopt;

	// -------------------------
	// ランダムに攻撃を決める
	// -------------------------
	int max = static_cast<int>(BossAttackType::Count) - 1; // Count分引く
	int			   r   = Random::Generate<int>(0,max);
	BossAttackType atk = static_cast<BossAttackType>(r);

	// 攻撃が決まった → クールダウン設定
	cooldownTimer_ = 2.5f;

	return atk;
}