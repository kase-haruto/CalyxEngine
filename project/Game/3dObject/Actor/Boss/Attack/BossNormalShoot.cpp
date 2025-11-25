#include "BossNormalShoot.h"

/* ========================================================================
/*	include space
/* ===================================================================== */
#include "../Boss.h"
#include <Game/Battle/Shooting/ShootingController/BossShootingController.h>

///////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
///////////////////////////////////////////////////////////////////////////////////////////
BossNormalShoot::BossNormalShoot()  = default;
BossNormalShoot::~BossNormalShoot() = default;

///////////////////////////////////////////////////////////////////////////////////////////
//		攻撃実行
///////////////////////////////////////////////////////////////////////////////////////////
bool BossNormalShoot::Execute(class Boss&                   boss,
								 class BossShootingController& shooter) const {

	// targetの方向に弾を撃つ
	const Vector3 bossPos = boss.GetCenterPos();
	const Vector3 playerPos = boss.GetTargetWorldPos();
	const Vector3 dir = (playerPos - bossPos).Normalize();

	// 発射Request
	shooter.RequestShoot(bossPos,dir);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////
//		デバッグ用gui
///////////////////////////////////////////////////////////////////////////////////////////
void BossNormalShoot::ShowGui() {
	ImGui::Text("Boss Normal Shoot");
}