#pragma once
#include <Game/3dObject/Actor/Bullet/Details/BulletDetails.h>
#include <Game/3dObject/Actor/Enemy/Enemy.h>
#include <Game/Battle/Shooting/Shooter/Interface/IBulletShooter.h>

#include <list>
#include <memory>

class BulletContainer;

class PlayerHomingBulletShooter
	: public IBulletShooter{
public:
	//===================================================================*/
	//						public functions
	//===================================================================*/
	PlayerHomingBulletShooter(BulletContainer* container, BulletID id);

	void Shoot(const CxMath::Vector3& origin, const CxMath::Vector3& direction = {})override;
	void SetTargets(const std::vector<std::shared_ptr<Enemy>>& targets){
		targets_ = targets;
	}

private:
	//===================================================================*/
	//						private variables
	//===================================================================*/
	BulletContainer* container_ = nullptr;
	BulletID         id_ = BulletID::Player_Homing;
	std::vector<std::shared_ptr<Enemy>> targets_;
};