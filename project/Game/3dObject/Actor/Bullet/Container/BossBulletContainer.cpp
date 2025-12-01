#include "BossBulletContainer.h"
#include <Game/3dObject/Actor/Bullet/Factory/BulletFactory.h>

BossBulletContainer::BossBulletContainer(const std::string& name) : BulletContainer(name) {}
BossBulletContainer::~BossBulletContainer() = default;

void BossBulletContainer::Update(float dt) {
	BulletContainer::Update(dt);
}

void BossBulletContainer::AddBullet(BulletID id, const Vector3& pos, const Vector3& vel) {

	// Boss 弾として許可するID
	switch(id) {
	case BulletID::Boss_Straight:
	case BulletID::Boss_Homing:
		break;
	default:
		return; // Boss 弾以外は受け付けない
	}

	// 生成
	auto bullet = BulletFactory::Create(id);
	if(!bullet) return;

	bullet->ShootInitialize(pos, vel);

	typedBullets_[id].push_back(bullet);
}

void BossBulletContainer::AddBullet(BulletID id, const std::shared_ptr<BaseBullet>& bullet) {
	switch(id) {
	case BulletID::Boss_Straight:
	case BulletID::Boss_Homing:
		break;
	default:
		return;
	}
	if (!bullet) return;

	typedBullets_[id].push_back(bullet);
}