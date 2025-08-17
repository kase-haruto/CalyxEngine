#include "EnemyBulletContainer.h"
#include <Game/3dObject/Actor/Bullet/Factory/BulletFactory.h>

EnemyBulletContainer::EnemyBulletContainer(const std::string& name):
	BulletContainer(name){}

void EnemyBulletContainer::AddBullet(BulletID id, const Vector3& pos, const Vector3& vel){
	if (id != BulletID::Enemy_Straight){return;}

	auto bullet = BulletFactory::Create(id);
	if (!bullet) return;
	bullet->ShootInitialize(pos, vel);
	typedBullets_[id].push_back(bullet);
}
