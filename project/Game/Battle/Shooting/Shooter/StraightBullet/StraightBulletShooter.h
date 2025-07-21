#pragma once

/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Game/Battle/Shooting/Shooter/Interface/IBulletShooter.h>
#include <Game/3dObject/Actor/Bullet/Container/BulletContainer.h>

class StraightBulletShooter :
	public IBulletShooter{
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	StraightBulletShooter(class BulletContainer* container,BulletID id);

	void Shoot(const Vector3& origin, const Vector3& direction) override;

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	BulletContainer* container_ = nullptr;
	BulletID bulletID_;
};

