#include "BossHomingSpreadShoot.h"

#include "Engine/Foundation/Math/Vector3.h"
#include "Engine/Foundation/Utility/Func/CxUtils.h"
#include "Game/3dObject/Actor/Boss/Boss.h"
#include "Game/3dObject/Actor/Bullet/BossBullet/BossHomingBullet.h"

///////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
///////////////////////////////////////////////////////////////////////////////////////////
BossHomingSpreadShoot::BossHomingSpreadShoot() = default;
BossHomingSpreadShoot::~BossHomingSpreadShoot() = default;

///////////////////////////////////////////////////////////////////////////////////////////
//		攻撃実行
///////////////////////////////////////////////////////////////////////////////////////////

bool BossHomingSpreadShoot::Execute(Boss& boss, BossShootingController& shooter) const {

	const CalyxMath::Vector3 bossPos = boss.GetCenterPos();

	const float startRad = CalyxMath::ToRadians(startAngleDeg_);
	const float stepRad  = CalyxMath::ToRadians(360.0f / bulletCount_);

	for (int i = 0; i < bulletCount_; i++) {

		float angle = startRad + stepRad * i;

		// 放射方向ベクトル
		CalyxMath::Vector3 dir = {
			std::cos(angle),
			0.0f,
			std::sin(angle)
		};

		// 弾生成
		auto bullet = shooter.AddBullet(
			BulletID::Boss_Homing,
			bossPos,
			dir * initialSpeed_          // ← 初速度
		);

		if (!bullet) continue;

		// ホーミング設定
		if (auto* homing = dynamic_cast<BossHomingBullet*>(bullet.get())) {
			homing->SetTarget(&boss.GetTargetActor()->GetWorldTransform());
			homing->SetHomingDelay(homingDelay_);
		}
	}

	return true;
}
///////////////////////////////////////////////////////////////////////////////////////////
//		デバッグ用gui
///////////////////////////////////////////////////////////////////////////////////////////
void BossHomingSpreadShoot::ShowGui() {
}