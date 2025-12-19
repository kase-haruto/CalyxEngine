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
	shootCooldown_ = 1.5f;
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
bool EnemyShootingController::RequestShoot(const CxMath::Vector3& pos, const CxMath::Vector3& dir) {
	if (!gameplayEngaged_) return false;

	if (!externalRateControl_) {
		if (shootCooldown_ >= 0.0f) return false;
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

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
//      インターバルを取得
/////////////////////////////////////////////////////////////////////////////////////////
float EnemyShootingController::GetInterval() const { return kInterval; }

void EnemyShootingController::SetBulletContainer(BulletContainer* container) {
	pBulletContainer_ = container;
}