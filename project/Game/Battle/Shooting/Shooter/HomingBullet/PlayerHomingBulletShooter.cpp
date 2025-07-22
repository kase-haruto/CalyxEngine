#include "PlayerHomingBulletShooter.h"

#include <Game/3dObject/Actor/Bullet/Container/PlayerBulletContainer.h>
#include <Game/3dObject/Actor/Bullet/Factory/BulletFactory.h>
#include <Game/3dObject/Actor/Bullet/PlayerBullet/HomingBullet.h>

PlayerHomingBulletShooter::PlayerHomingBulletShooter(BulletContainer* container, BulletID id)
	: container_(container), id_(id){}

void PlayerHomingBulletShooter::Shoot(const Vector3& origin,
									  [[maybe_unused]] const Vector3& direction){
	if (!container_ || targets_.empty()) return;

	auto* playerContainer = dynamic_cast< PlayerBulletContainer* >(container_);
	if (!playerContainer) return;

	for (const auto& target : targets_){
		if (!target) continue;

		auto bullet = BulletFactory::Create(id_);
		if (!bullet) continue;

		bullet->SetScale(Vector3(0.3f, 0.3f, 0.3f));
		bullet->ShootInitialize(origin, Vector3::Zero());

		if (auto* homing = dynamic_cast< HomingBullet* >(bullet.get())){
			homing->SetTarget(&target->GetWorldTransform());
		}
		playerContainer->AddBullet(id_, bullet);
	}
}