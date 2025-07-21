#include "PlayerShootingController.h"

// game
#include <Game/3dObject/Actor/Bullet/Container/BulletContainer.h>

// engine
#include <Engine/Foundation/Math/Vector3.h>

PlayerShootingController::PlayerShootingController(BulletContainer* container){
	straightShooter_ = std::make_unique<StraightBulletShooter>(container, BulletID::Player_Straight);
	// homingShooter_ = std::make_unique<HomingBulletShooter>();
}

void PlayerShootingController::Update(float dt){
	shootCooldown_ -= dt;
}

void PlayerShootingController::RequestShoot(const Vector3& pos, const Vector3& dir){
	if (shootCooldown_ > 0.0f) return;

	switch (bulletMode_){
		case PlayerShoot::BulletMode::Straight:
			RequestShootStraight(pos, dir);
			break;
		case PlayerShoot::BulletMode::Homing:
			RequestShootHoming(pos, dir);
			break;
		default:
			break;
	}

	shootCooldown_ = kShootInterval_;
}

void PlayerShootingController::SetMode(PlayerShoot::BulletMode bulletMode){
	bulletMode_ = bulletMode;
}

//=========================
// Private
//=========================

void PlayerShootingController::RequestShootStraight(const Vector3& pos, const Vector3& dir){
	if (straightShooter_){
		straightShooter_->Shoot(pos, dir);
	}
}

void PlayerShootingController::RequestShootHoming([[maybe_unused]]const Vector3& pos,
												  [[maybe_unused]] const Vector3& dir){
	/*if (homingShooter_){
		homingShooter_->Shoot(pos, dir);
	}*/
}
