#include "EnemyBulletContainer.h"
#include <Game/3dObject/Actor/Bullet/Factory/BulletFactory.h>

EnemyBulletContainer::EnemyBulletContainer(const std::string& name):
	BulletContainer(name){}

void EnemyBulletContainer::AddBullet(BulletID id, const CxMath::Vector3& pos, const CxMath::Vector3& vel){
	if (id != BulletID::Enemy_Straight){return;}

	auto bullet = BulletFactory::Create(id);
	if (!bullet) return;
	bullet->ShootInitialize(pos, vel);
	typedBullets_[id].push_back(bullet);
}

void EnemyBulletContainer::AddBullet(BulletID id, const std::shared_ptr<BaseBullet>& bullet) {
	if (id != BulletID::Enemy_Homing && id != BulletID::Enemy_Homing) return;
	if (!bullet) return;
	typedBullets_[id].push_back(bullet);
}