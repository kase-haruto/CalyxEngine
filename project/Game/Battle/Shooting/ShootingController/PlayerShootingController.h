#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
// game
#include <Game/Battle/Shooting/Shooter/StraightBullet/StraightBulletShooter.h>

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
	PlayerShoot::BulletMode bulletMode_;

private:
	float shootCooldown_ = 0.0f;			//< クールダウン
	const float kShootInterval_ = 0.3f;		//< インターバル
};