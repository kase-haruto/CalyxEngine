#pragma once

/* ========================================================================
/*		include space
/* ===================================================================== */
#include <Game/Battle/Shooting/ShootingController/BaseShootingController.h>
#include <Game/Battle/Shooting/Shooter/StraightBullet/StraightBulletShooter.h>

class BulletContainer;

class EnemyShootingController final :
		public BaseShootingController{
public:
	//===================================================================*/
	//			public method
	//===================================================================*/
	EnemyShootingController(BulletContainer* container);
	~EnemyShootingController() override = default;

	void RequestShoot(const Vector3& pos, const Vector3& dir) override;
	float GetInterval() const override;

private:
	std::unique_ptr<StraightBulletShooter> straightShooter_ = nullptr;
	
	float kInterval = 1.5f;
};
