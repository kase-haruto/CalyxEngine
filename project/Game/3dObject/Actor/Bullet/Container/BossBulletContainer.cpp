#include "BossBulletContainer.h"
#include <Game/3dObject/Actor/Bullet/Factory/BulletFactory.h>

BossBulletContainer::BossBulletContainer(const std::string& name) :
	BulletContainer(name) {}

void BossBulletContainer::Update(float dt) {
	BulletContainer::Update(dt);
}

void BossBulletContainer::AddBullet(BulletID id, const Vector3& pos, const Vector3& vel) {
	if (id != BulletID::Boss_Straight) { return; }

	auto bullet = BulletFactory::Create(id);
	if (!bullet) return;
	bullet->ShootInitialize(pos, vel);
	typedBullets_[id].push_back(bullet);
}
