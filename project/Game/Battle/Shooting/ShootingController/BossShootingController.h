#pragma once

/* ========================================================================
/*		include space
/* ===================================================================== */
#include "Game/3dObject/Actor/Bullet/Container/BossBulletContainer.h"
#include <Game/Battle/Shooting/Shooter/StraightBullet/StraightBulletShooter.h>
#include <Game/Battle/Shooting/ShootingController/BaseShootingController.h>

class BulletContainer;

class BossShootingController final : public BaseShootingController {
public:
	//===================================================================*/
	//			public method
	//===================================================================*/
	BossShootingController(std::unique_ptr<BossBulletContainer> container);
	~BossShootingController() override;

	//--------- main ------------------------------------------------------
	void Update(float dt) override;
	bool RequestShoot(const Vector3& pos, const Vector3& dir) override;

	/**
	 * \brief 弾の生成
	 * \param id 弾のID
	 * \param pos 生成位置
	 * \param vel 初速度
	 * \return 生成した弾のshared_ptr
	 */
	std::shared_ptr<BaseBullet> AddBullet(BulletID id, const Vector3& pos, const Vector3& vel);

	//--------- accessor --------------------------------------------------
	float GetInterval() const override;
	BulletContainer* GetBulletContainer() const{return bulletContainer_.get();}

private:
	std::unique_ptr<StraightBulletShooter> straightShooter_ = nullptr;
	std::unique_ptr<BossBulletContainer>	   bulletContainer_ = nullptr;

	float kInterval = 1.5f;
};