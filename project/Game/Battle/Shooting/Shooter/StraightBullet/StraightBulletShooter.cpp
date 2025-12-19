#include "StraightBulletShooter.h"

#include <Game/3dObject/Actor/Bullet/Container/BulletContainer.h>
#include <Game/3dObject/Actor/Bullet/Details/BulletDetails.h>

StraightBulletShooter::StraightBulletShooter(BulletContainer* container, BulletID id)
	: container_(container),bulletID_(id){}

void StraightBulletShooter::Shoot(const CalyxMath::Vector3& origin, const CalyxMath::Vector3& direction){
	if (!container_) return;
	container_->AddBullet(bulletID_, origin, direction);
}