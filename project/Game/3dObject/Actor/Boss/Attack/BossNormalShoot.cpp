#include "BossNormalShoot.h"

/* ========================================================================
/*	include space
/* ===================================================================== */
#include "../Boss.h"
#include "Game/3dObject/Actor/Bullet/BossBullet/BossHomingBullet.h"

#include <Game/Battle/Shooting/ShootingController/BossShootingController.h>

///////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
///////////////////////////////////////////////////////////////////////////////////////////
BossNormalShoot::BossNormalShoot()	= default;
BossNormalShoot::~BossNormalShoot() = default;

///////////////////////////////////////////////////////////////////////////////////////////
//		攻撃実行
///////////////////////////////////////////////////////////////////////////////////////////
bool BossNormalShoot::Execute(Boss& boss, BossShootingController& shooter) const {

	// ボスの位置
	const Vector3 bossPos = boss.GetCenterPos();

	// プレイヤー方向（初速）
	const Vector3 playerPos = boss.GetTargetWorldPos();
	const Vector3 dir = (playerPos - bossPos).Normalize();

	// ★ Boss ホーミング弾を生成
	auto bullet = shooter.AddBullet(
		BulletID::Boss_Homing,      // ← ★ ホーミング弾
		bossPos, 
		dir * 1.0f                  // ← 初速（適当でOK）
	);

	// ★ ホーミング弾なら追尾ターゲットを設定
	if (auto* homing = dynamic_cast<BossHomingBullet*>(bullet.get())) {
		homing->SetTarget(boss.GetTargetActor());
		homing->SetHomingDelay(0.0f); // 今回は即ホーミング開始
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////
//		デバッグ用gui
///////////////////////////////////////////////////////////////////////////////////////////
void BossNormalShoot::ShowGui() {
	ImGui::Text("Boss Normal Shoot");
}