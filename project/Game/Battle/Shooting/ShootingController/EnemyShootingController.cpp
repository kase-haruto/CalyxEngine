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
EnemyShootingController::EnemyShootingController(BulletContainer* container) {
	SetBulletContainer(container);
	homingShooter = std::make_unique<EnemyHomingBulletShooter>(container, BulletID::Enemy_Homing);
}

EnemyShootingController::~EnemyShootingController() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//      更新
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyShootingController::Update(float dt) {
	if (!gameplayEngaged_) {
		return;
	}

	// ベース側で shootCooldown_ の減算などを実施
	BaseShootingController::Update(dt);
}

/////////////////////////////////////////////////////////////////////////////////////////
//      発射Request
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyShootingController::RequestShoot(const Vector3& pos, const Vector3& dir) {
	if (!gameplayEngaged_) return;

	if (!externalRateControl_) {
		if (shootCooldown_ >= 0.0f) return;
	}

	if (auto ctx = SceneContext::Current()) {
		if (auto player = ctx->FindFirst<Player>()) {
			homingShooter->SetTarget(player);
		}
	}

	homingShooter->Shoot(pos, dir);

	if (!externalRateControl_) {
		shootCooldown_ = GetInterval();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//      インターバルを取得
/////////////////////////////////////////////////////////////////////////////////////////
float EnemyShootingController::GetInterval() const { return kInterval; }

void EnemyShootingController::SetBulletContainer(BulletContainer* container) {
	pBulletContainer_ = container;
}