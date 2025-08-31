#include "EnemyHomingBulletShooter.h"

#include <Game/3dObject/Actor/Bullet/Container/EnemyBulletContainer.h>
#include <Game/3dObject/Actor/Bullet/Factory/BulletFactory.h>
#include <Game/3dObject/Actor/Bullet/playerBullet/HomingBullet.h>
#include <Game/3dObject/Actor/Bullet/EnemyBullet/EnemyHomingBullet.h>
#include <Engine/Objects/3D/Actor/Actor.h>

EnemyHomingBulletShooter::EnemyHomingBulletShooter(BulletContainer* container, BulletID id)
	: container_(container), id_(id) {}

void EnemyHomingBulletShooter::Shoot(const Vector3& origin,
									 [[maybe_unused]] const Vector3& direction) {
	if (!container_ || !target_) return;

	auto* enemyContainer = dynamic_cast<EnemyBulletContainer*>(container_);
	if (!enemyContainer) return;


	auto bullet = BulletFactory::Create(id_);

	bullet->SetScale(Vector3(0.3f, 0.3f, 0.3f));
	Vector3 toTarget = target_->GetWorldPosition() - origin;
	if (toTarget.Length() > 0.001f) {
		toTarget.Normalize();
	} else {
		toTarget = Vector3(0, 0, 1);
	}
	bullet->ShootInitialize(origin, toTarget);

	if (auto* homing = dynamic_cast<EnemyHomingBullet*>(bullet.get())) {
		homing->SetTarget(target_.get());
	}
	enemyContainer->AddBullet(id_, bullet);
}