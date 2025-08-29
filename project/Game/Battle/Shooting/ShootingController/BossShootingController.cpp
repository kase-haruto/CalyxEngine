#include "BossShootingController.h"

/* ========================================================================
/*  include space
/* ===================================================================== */

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
BossShootingController::BossShootingController(std::unique_ptr<BulletContainer> container) {
	bulletContainer_ = std::move(container);
	straightShooter_ = std::make_unique<StraightBulletShooter>(bulletContainer_.get(), BulletID::Enemy_Straight);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void BossShootingController::Update(float dt) {
	bulletContainer_->Update(dt);
	BaseShootingController::Update(dt);
}


/////////////////////////////////////////////////////////////////////////////////////////
//		発射Request
/////////////////////////////////////////////////////////////////////////////////////////
void BossShootingController::RequestShoot([[maybe_unused]] const Vector3& pos, [[maybe_unused]] const Vector3& dir) {
	if (shootCooldown_ >= 0) { return; }

	straightShooter_->Shoot(pos, dir);

	shootCooldown_ = GetInterval();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		インターバルを取得
/////////////////////////////////////////////////////////////////////////////////////////
float BossShootingController::GetInterval() const { return kInterval; }

void BossShootingController::SetBulletContainer(std::unique_ptr<BulletContainer> container) { bulletContainer_ = std::move(container); }
