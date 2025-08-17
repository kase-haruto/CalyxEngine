#include "EnemyShootingController.h"
/* ========================================================================
/*  include space
/* ===================================================================== */

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
EnemyShootingController::EnemyShootingController(std::unique_ptr<BulletContainer> container){
	bulletContainer_ = std::move(container);
	straightShooter_ = std::make_unique<StraightBulletShooter>(bulletContainer_.get(),BulletID::Enemy_Straight);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyShootingController::Update(float dt){
	bulletContainer_->Update(dt);
	BaseShootingController::Update(dt);
}


/////////////////////////////////////////////////////////////////////////////////////////
//		発射Request
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyShootingController::RequestShoot([[maybe_unused]] const Vector3& pos, [[maybe_unused]] const Vector3& dir){
	if (shootCooldown_ >= 0){ return; }

	straightShooter_->Shoot(pos,dir);

	shootCooldown_ = GetInterval();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		インターバルを取得
/////////////////////////////////////////////////////////////////////////////////////////
float EnemyShootingController::GetInterval() const{ return kInterval; }

void EnemyShootingController::SetBulletContainer(std::unique_ptr<BulletContainer> container){bulletContainer_ = std::move(container);}
