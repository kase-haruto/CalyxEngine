#include "PlayerHomingBulletShooter.h"

#include <Engine/Foundation/Utility/Ease/CxEase.h>

#include <Game/3dObject/Actor/Bullet/Container/PlayerBulletContainer.h>
#include <Game/3dObject/Actor/Bullet/Factory/BulletFactory.h>
#include <Game/3dObject/Actor/Bullet/PlayerBullet/HomingBullet.h>

PlayerHomingBulletShooter::PlayerHomingBulletShooter(BulletContainer* container, BulletID id)
	: container_(container), id_(id){}

void PlayerHomingBulletShooter::Shoot(const CalyxMath::Vector3& origin,
									  [[maybe_unused]] const CalyxMath::Vector3& direction){
	if (!container_ || targets_.empty()) return;

	auto* playerContainer = dynamic_cast< PlayerBulletContainer* >(container_);
	if (!playerContainer) return;

	for (const auto& target : targets_){
		if (!target) continue;

		auto bullet = BulletFactory::Create(id_);
		if (!bullet) continue;

		bullet->SetScale(CalyxMath::Vector3(0.3f, 0.3f, 0.3f));
		CalyxMath::Vector3 toTarget = target->GetWorldPosition() - origin;
		if (toTarget.Length() > 0.001f){
			toTarget = toTarget.Normalize();
		} else{
			toTarget = CalyxMath::Vector3(0, 0, 1);
		}
		bullet->ShootInitialize(origin, toTarget);

		if (auto* homing = dynamic_cast< HomingBullet* >(bullet.get())){
			homing->SetTarget(target.get());
			// 速度イージングの設定 (初速, 終速, 時間, タイプ)
			homing->SetSpeedEase(1.0f, 7.0f, 1.0f);
		}
		playerContainer->AddBullet(id_, bullet);
	}
}