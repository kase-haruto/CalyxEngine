#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
// game
#include <Game/Battle/Shooting/Shooter/StraightBullet/StraightBulletShooter.h>
#include <Game/Battle/Shooting/Shooter/HomingBullet/PlayerHomingBulletShooter.h>
#include <Game/3dObject/Actor/Enemy/Enemy.h>
#include <Game/Battle/Shooting/ShootingController/BaseShootingController.h>

// c++
#include <memory>

// forward
struct CalyxMath::Vector3;
class BulletContainer;

namespace PlayerShoot{
enum class BulletMode{
	Straight,
	Homing,
};
}

class PlayerShootingController final :
		public BaseShootingController{
public:
	//===================================================================*/
	//						public functions
	//===================================================================*/
	PlayerShootingController(BulletContainer* container);
	PlayerShootingController() = default;
	~PlayerShootingController()override = default;

	bool RequestShoot(const CalyxMath::Vector3& pos, const CalyxMath::Vector3& dir)override;
	void Initialize();
	void Update(float dt)override;
	//--------- accessor -------------------------------------------------
	void SetMode(PlayerShoot::BulletMode bulletMode);
	void SetTargets(const std::vector<std::shared_ptr<Enemy>>& targets);

	float GetInterval()const override{ return kShootInterval_; }

	void SetBulletContainer(std::unique_ptr<BulletContainer> container);
private:
	//===================================================================*/
	//						private functions
	//===================================================================*/
	void RequestShootStraight(const CalyxMath::Vector3& pos, const CalyxMath::Vector3& dir);
	void RequestShootHoming(const CalyxMath::Vector3& pos, const CalyxMath::Vector3& dir);

private:
	//===================================================================*/
	//						private variables
	//===================================================================*/
	std::unique_ptr<StraightBulletShooter> straightShooter_;
	std::unique_ptr<PlayerHomingBulletShooter> homingShooter_;
	PlayerShoot::BulletMode bulletMode_;

	std::unique_ptr<BulletContainer> bulletContainer_;
private:
	static constexpr float kShootInterval_ = 0.45f;
};