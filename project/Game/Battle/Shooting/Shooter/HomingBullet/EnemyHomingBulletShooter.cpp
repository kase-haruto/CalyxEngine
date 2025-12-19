#include "EnemyHomingBulletShooter.h"

#include <Game/3dObject/Actor/Bullet/Container/EnemyBulletContainer.h>
#include <Game/3dObject/Actor/Bullet/Factory/BulletFactory.h>
#include <Game/3dObject/Actor/Bullet/playerBullet/HomingBullet.h>
#include <Game/3dObject/Actor/Bullet/EnemyBullet/EnemyHomingBullet.h>
#include <Engine/Objects/3D/Actor/Actor.h>

EnemyHomingBulletShooter::EnemyHomingBulletShooter(BulletContainer* container, BulletID id)
	: container_(container), id_(id) {}

void EnemyHomingBulletShooter::Shoot(const CalyxMath::Vector3& origin,
									 const CalyxMath::Vector3& direction) {
	if (!container_ || !target_) return;

	auto* enemyContainer = dynamic_cast<EnemyBulletContainer*>(container_);
	if (!enemyContainer) return;

	auto bullet = BulletFactory::Create(id_);
	bullet->SetScale(CalyxMath::Vector3(0.3f, 0.3f, 0.3f));

	CalyxMath::Vector3 initDir = direction;
	if (initDir.LengthSquared() <= 1e-6f) {
		initDir = target_->GetWorldPosition() - origin; // フォールバック
	}
	if (initDir.LengthSquared() > 1e-6f) initDir = initDir.Normalize();
	else                                  initDir = CalyxMath::Vector3(0, 0, 1);

	bullet->ShootInitialize(origin, initDir);

	if (auto* homing = dynamic_cast<EnemyHomingBullet*>(bullet.get())) {
		homing->SetTarget(target_.get());
		// homing->SetInitialLockTime(0.12f);
		// homing->SetActivationDelay(0.06f);
		// homing->SetMaxTurnRateRad(2.0f);
	}

	enemyContainer->AddBullet(id_, bullet);
}