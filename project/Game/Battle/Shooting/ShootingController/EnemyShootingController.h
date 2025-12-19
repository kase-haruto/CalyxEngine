#pragma once

/* ========================================================================
/*      include space
/* ===================================================================== */
#include <Game/Battle/Shooting/ShootingController/BaseShootingController.h>
#include <Game/Battle/Shooting/Shooter/StraightBullet/StraightBulletShooter.h>

class BulletContainer;
class EnemyHomingBulletShooter;

class EnemyShootingController final
	: public BaseShootingController {
public:
	//===================================================================*/
	//          public method
	//===================================================================*/
	EnemyShootingController(BulletContainer* container);
	~EnemyShootingController() override;

	//--------- main ------------------------------------------------------
	void Update(float dt) override;
	bool RequestShoot(const CxMath::Vector3& pos, const CxMath::Vector3& dir) override;

	//--------- accessor --------------------------------------------------
	float GetInterval() const override;
	void SetBulletContainer(BulletContainer* container);

	void SetExternalRateControl(bool v) { externalRateControl_ = v; }

	void SetGameplayEngaged(bool v) { gameplayEngaged_ = v; }
	bool IsGameplayEngaged() const { return gameplayEngaged_; }

private:
	std::unique_ptr<StraightBulletShooter>  straightShooter_ = nullptr;
	std::unique_ptr<EnemyHomingBulletShooter> homingShooter = nullptr;
	BulletContainer* pBulletContainer_ = nullptr;

	float kInterval = 1.2f;

	bool gameplayEngaged_ = true;
	bool externalRateControl_ = false; //< trueなら内部クールダウン無効
};