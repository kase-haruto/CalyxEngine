#pragma once
#include <Game/3dObject/Actor/Bullet/Details/BulletDetails.h>
#include <Game/Battle/Shooting/Shooter/Interface/IBulletShooter.h>
#include <Engine/Foundation/Math/Vector3.h>

#include <list>
#include <memory>

class BulletContainer;
class Actor;

class EnemyHomingBulletShooter
	: public IBulletShooter {
public:
	//===================================================================*/
	//						public functions
	//===================================================================*/
	EnemyHomingBulletShooter(BulletContainer* container, BulletID id);
	void Shoot(const CalyxMath::Vector3& origin, const CalyxMath::Vector3& direction = {})override;
	void SetTarget(const std::shared_ptr<Actor>& target) {
		target_ = target;
	}

private:
	//===================================================================*/
	//						private variables
	//===================================================================*/
	BulletContainer* container_ = nullptr;
	BulletID id_ = BulletID::Enemy_Homing;
	std::shared_ptr<Actor> target_;
};