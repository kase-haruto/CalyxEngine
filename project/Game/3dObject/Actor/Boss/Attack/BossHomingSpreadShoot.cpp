#include "BossHomingSpreadShoot.h"

#include "Engine/Foundation/Math/Vector3.h"
#include "Engine/Foundation/Utility/Func/CxUtils.h"
#include "Game/3dObject/Actor/Boss/Boss.h"
#include "Game/3dObject/Actor/Bullet/BossBullet/BossHomingBullet.h"

///////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
///////////////////////////////////////////////////////////////////////////////////////////
BossHomingSpreadShoot::BossHomingSpreadShoot(){
	SetAttackName("HomingSpreadShoot");

	// パラメータ読み込み 名前同期
	param_.SetAttackName(GetAttackName());
	param_.LoadParams();
}

BossHomingSpreadShoot::~BossHomingSpreadShoot() = default;

///////////////////////////////////////////////////////////////////////////////////////////
//		攻撃実行
///////////////////////////////////////////////////////////////////////////////////////////
bool BossHomingSpreadShoot::Execute(Boss& boss,BossShootingController& shooter) const {

	const CalyxMath::Vector3 bossPos = boss.GetCenterPos();

	const float startRad = CalyxMath::ToRadians(param_.startAngleDeg);
	const float stepRad  = CalyxMath::ToRadians(360.0f / param_.bulletCount);

	for(int i = 0; i < param_.bulletCount; i++) {
		float angle = startRad + stepRad * i;

		// 放射方向ベクトル
		// Z軸中心（XY平面）への変更
		CalyxMath::Vector3 dir = {
			std::cos(angle),
			std::sin(angle),
			0.0f
		   };

		// 弾生成
		auto bullet = shooter.AddBullet(
			BulletID::Boss_Homing,
			bossPos,
			dir * param_.initialSpeed // ← 初速度
			);

		if(!bullet) continue;

		// ホーミング設定
		if(auto* homing = dynamic_cast<BossHomingBullet*>(bullet.get())) {
			homing->SetTargetPosition(boss.GetTargetActor()->GetCenterPos());
			homing->SetHomingDelay(param_.homingDelay);
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////
//		デバッグ用gui
///////////////////////////////////////////////////////////////////////////////////////////
void BossHomingSpreadShoot::ShowGui() { param_.ShowGui(); }

BossHomingSpreadShoot::BossHomigSqreadShootParam::BossHomigSqreadShootParam(){
	AddField("bulletCount",bulletCount).Category("BossHomingSpreadShoot").Range(1,12).Tooltip("撒く弾の数");
	AddField("initialSpeed",initialSpeed).Category("BossHomingSpreadShoot").Range(0.1f,10.0f).Tooltip("直進スピード");
	AddField("homingDelay",homingDelay).Category("BossHomingSpreadShoot").Range(0.0f,5.0f).Tooltip("ホーミング開始遅延(秒)");
	AddField("startAngleDeg",startAngleDeg).Category("BossHomingSpreadShoot").Range(0.0f,360.0f).Tooltip("撒く角度");
}