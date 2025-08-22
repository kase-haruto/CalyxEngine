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
	EnemyShootingController(std::unique_ptr<BulletContainer> container);
	~EnemyShootingController() override = default;

	//--------- main ------------------------------------------------------
	void Update(float dt) override;
	void RequestShoot(const Vector3& pos, const Vector3& dir) override;

	//--------- accessor --------------------------------------------------
	float GetInterval() const override;
	void SetBulletContainer(std::unique_ptr<BulletContainer> container);
private:
	std::unique_ptr<StraightBulletShooter> straightShooter_ = nullptr;
	std::unique_ptr<BulletContainer> bulletContainer_ = nullptr;
	
	float kInterval = 1.5f;
};
