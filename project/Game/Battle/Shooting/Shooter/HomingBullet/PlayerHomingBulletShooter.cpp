#include "PlayerHomingBulletShooter.h"

#include <Game/3dObject/Actor/Bullet/Container/PlayerBulletContainer.h>
#include <Game/3dObject/Actor/Bullet/Factory/BulletFactory.h>
#include <Game/3dObject/Actor/Bullet/PlayerBullet/HomingBullet.h>

PlayerHomingBulletShooter::PlayerHomingBulletShooter(BulletContainer* container, BulletID id)
	: container_(container), id_(id){}

void PlayerHomingBulletShooter::Shoot(const CxMath::Vector3& origin,
									  [[maybe_unused]] const CxMath::Vector3& direction){
	if (!container_ || targets_.empty()) return;

	auto* playerContainer = dynamic_cast< PlayerBulletContainer* >(container_);
	if (!playerContainer) return;

	for (const auto& target : targets_){
		if (!target) continue;

		auto bullet = BulletFactory::Create(id_);
		if (!bullet) continue;

		bullet->SetScale(CxMath::Vector3(0.3f, 0.3f, 0.3f));
		CxMath::Vector3 toTarget = target->GetWorldPosition() - origin;
		if (toTarget.Length() > 0.001f){
			toTarget.Normalize();
		} else{
			toTarget = CxMath::Vector3(0, 0, 1);
		}
		bullet->ShootInitialize(origin, toTarget);

		if (auto* homing = dynamic_cast< HomingBullet* >(bullet.get())){
			homing->SetTarget(target.get());
		}
		playerContainer->AddBullet(id_, bullet);
	}
}