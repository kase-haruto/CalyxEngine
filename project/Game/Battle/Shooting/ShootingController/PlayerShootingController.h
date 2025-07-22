#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
// game
#include <Game/Battle/Shooting/Shooter/StraightBullet/StraightBulletShooter.h>
#include <Game/Battle/Shooting/Shooter/HomingBullet/PlayerHomingBulletShooter.h>
#include <Game/3dObject/Actor/Enemy/Enemy.h>

// c++
#include <memory>

// forward
struct Vector3;
class BulletContainer;

namespace PlayerShoot{
	enum class BulletMode{
		Straight,
		Homing,
	};
}

class PlayerShootingController{
public:
	//===================================================================*/
	//						public functions
	//===================================================================*/
	PlayerShootingController(BulletContainer* container);
	void Update(float dt);
	void RequestShoot(const Vector3& pos, const Vector3& dir);


	//--------- accessor -------------------------------------------------
	void SetMode(PlayerShoot::BulletMode bulletMode);
	void SetTargets(const std::vector<std::shared_ptr<Enemy>>& targets);

	float GetCooldown(){ return shootCooldown_; }
	float GetInterval(){ return kShootInterval_; }

private:
	//===================================================================*/
	//						private functions
	//===================================================================*/
	void RequestShootStraight(const Vector3& pos, const Vector3& dir);
	void RequestShootHoming(const Vector3& pos, const Vector3& dir);

private:
	//===================================================================*/
	//						private variables
	//===================================================================*/
	std::unique_ptr<StraightBulletShooter> straightShooter_;
	std::unique_ptr< PlayerHomingBulletShooter> homingShooter_;
	PlayerShoot::BulletMode bulletMode_;

private:
	float shootCooldown_ = 0.0f;			//< クールダウン
	static constexpr float kShootInterval_ = 0.3f;
};