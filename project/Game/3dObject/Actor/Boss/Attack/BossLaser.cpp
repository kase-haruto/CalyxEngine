#include "BossLaser.h"

#include "Engine/Foundation/Math/Vector3.h"
#include "Game/3dObject/Actor/Boss/Boss.h"
#include "Game/3dObject/Actor/Bullet/BossBullet/BossHomingBullet.h"

BossLaser::BossLaser() = default;
BossLaser::~BossLaser() = default;

/////////////////////////////////////////////////////////////////////////////////////////////
//		攻撃実行
/////////////////////////////////////////////////////////////////////////////////////////////
bool BossLaser::Execute(Boss& boss, BossShootingController& shooter) const {

	// いったんホーミング弾を撃つように作成
	
	// ボスの位置
	const CalyxMath::Vector3 bossPos = boss.GetCenterPos();

	// プレイヤー方向（初速）
	const CalyxMath::Vector3 playerPos = boss.GetTargetWorldPos();
	const CalyxMath::Vector3 dir = (playerPos - bossPos).Normalize();

	// Boss ホーミング弾を生成
	auto bullet = shooter.AddBullet(
		BulletID::Boss_Homing,
		bossPos, 
		dir * 1.0f
	);

	// ホーミング弾なら追尾ターゲットを設定
	if (auto* homing = dynamic_cast<BossHomingBullet*>(bullet.get())) {
		homing->SetTarget(&boss.GetWorldTransform());
		homing->SetHomingLimit(1.0f);
		homing->SetHomingDelay(0.0f);
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//		デバッグ用gui
/////////////////////////////////////////////////////////////////////////////////////////////
void BossLaser::ShowGui() {
}