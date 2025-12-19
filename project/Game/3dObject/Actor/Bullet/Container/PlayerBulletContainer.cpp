#include "PlayerBulletContainer.h"
#include <Game/3dObject/Actor/Bullet/Factory/BulletFactory.h>

/////////////////////////////////////////////////////////////////////////////////////////
// コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
PlayerBulletContainer::PlayerBulletContainer(const std::string name) :
	BulletContainer(name){

}

/////////////////////////////////////////////////////////////////////////////////////////
//		弾追加
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerBulletContainer::AddBullet(BulletID id, const CxMath::Vector3& pos, const CxMath::Vector3& vel){
	if (id != BulletID::Player_Straight && id != BulletID::Player_Homing) return;
	auto bullet = BulletFactory::Create(id);
	if (!bullet) return;
	bullet->ShootInitialize(pos, vel);
	typedBullets_[id].push_back(bullet);
}

void PlayerBulletContainer::AddBullet(BulletID id, const std::shared_ptr<BaseBullet>& bullet){
	if (id != BulletID::Player_Straight && id != BulletID::Player_Homing) return;
	if (!bullet) return;
	typedBullets_[id].push_back(bullet);
}
