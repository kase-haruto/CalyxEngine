#include "EnemyShootingController.h"

/* ========================================================================
/*  include space
/* ===================================================================== */

#include <Engine/Scene/Context/SceneContext.h>
#include <Game/3dObject/Actor/Player/Player.h>
#include <Game/Battle/Shooting/Shooter/HomingBullet/EnemyHomingBulletShooter.h>

/////////////////////////////////////////////////////////////////////////////////////////
//      コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
EnemyShootingController::EnemyShootingController(std::unique_ptr<BulletContainer> container) {
	bulletContainer_ = std::move(container);
	homingShooter = std::make_unique<EnemyHomingBulletShooter>(bulletContainer_.get(), BulletID::Enemy_Homing);
}

EnemyShootingController::~EnemyShootingController() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//      更新
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyShootingController::Update(float dt) {
	if (bulletContainer_) bulletContainer_->Update(dt);

	if (!gameplayEngaged_) {
		return;
	}

	BaseShootingController::Update(dt);
}

/////////////////////////////////////////////////////////////////////////////////////////
//      発射Request
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyShootingController::RequestShoot(const Vector3& pos, const Vector3& dir) {
	if (!gameplayEngaged_) return;
	if (shootCooldown_ >= 0) return;

	if (auto ctx = SceneContext::Current()) {
		if (auto player = ctx->FindFirst<Player>()) {
			homingShooter->SetTarget(player);
		}
	}

	homingShooter->Shoot(pos, dir);
	shootCooldown_ = GetInterval();
}

/////////////////////////////////////////////////////////////////////////////////////////
//      インターバルを取得
/////////////////////////////////////////////////////////////////////////////////////////
float EnemyShootingController::GetInterval() const { return kInterval; }

void EnemyShootingController::SetBulletContainer(std::unique_ptr<BulletContainer> container) {
	bulletContainer_ = std::move(container);
}
