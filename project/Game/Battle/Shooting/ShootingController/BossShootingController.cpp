#include "BossShootingController.h"

#include "Game/3dObject/Actor/Bullet/Factory/BulletFactory.h"

/* ========================================================================
/*  include space
/* ===================================================================== */

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
BossShootingController::BossShootingController(std::unique_ptr<BossBulletContainer> container) {
	bulletContainer_ = std::move(container);
	straightShooter_ = std::make_unique<StraightBulletShooter>(bulletContainer_.get(), BulletID::Boss_Straight);
}
BossShootingController::~BossShootingController() = default;

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
bool BossShootingController::RequestShoot([[maybe_unused]] const Vector3& pos, [[maybe_unused]] const Vector3& dir) {
	if(shootCooldown_ >= 0) {
		return false;
	}

	straightShooter_->Shoot(pos, dir);

	shootCooldown_ = GetInterval();
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		弾の生成
/////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<BaseBullet> BossShootingController::AddBullet(BulletID id, const Vector3& pos, const Vector3& vel) {
	// BulletContainer より生成を委譲
	auto bullet = BulletFactory::Create(id);
	if(!bullet) return nullptr;

	bullet->ShootInitialize(pos, vel);

	bulletContainer_->AddBullet(id, bullet);
	return bullet;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		インターバルを取得
/////////////////////////////////////////////////////////////////////////////////////////
float BossShootingController::GetInterval() const { return kInterval; }
