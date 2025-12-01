#include "BossHomingSpreadShoot.h"

#include "Engine/Foundation/Math/Vector3.h"
#include "Engine/Foundation/Utility/Func/CxUtils.h"
#include "Game/3dObject/Actor/Boss/Boss.h"

///////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
///////////////////////////////////////////////////////////////////////////////////////////
BossHomingSpreadShoot::BossHomingSpreadShoot() {
	
}
BossHomingSpreadShoot::~BossHomingSpreadShoot() = default;

///////////////////////////////////////////////////////////////////////////////////////////
//		攻撃実行
///////////////////////////////////////////////////////////////////////////////////////////
bool BossHomingSpreadShoot::Execute(class Boss& , class BossShootingController& ) const {
	// const Vector3 bossPos = boss.GetCenterPos();

	const float startRad = Cx::Math::ToRadians(startAngleDeg_);
	const float stepRad  = Cx::Math::ToRadians(360.0f / bulletCount_);

	for (int i = 0; i < bulletCount_; i++) {

		float angle = startRad + stepRad * i;

		// 放射方向ベクトル
		Vector3 dir = {
			std::cos(angle),
			0.0f,
			std::sin(angle)
		};

		// 生成
		// auto bullet = shooter.CreateBullet<BossHomingBullet>();
		// if (!bullet) continue;
		//
		// bullet->SetTarget(boss.GetTarget());
		// bullet->SetHomingDelay(homingDelay_);
		// bullet->ShootInitialize(bossPos, dir * initialSpeed_);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////
//		デバッグ用gui
///////////////////////////////////////////////////////////////////////////////////////////
void BossHomingSpreadShoot::ShowGui() {
}