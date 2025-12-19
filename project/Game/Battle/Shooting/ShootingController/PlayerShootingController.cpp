#include "PlayerShootingController.h"

// game
#include <Game/3dObject/Actor/Bullet/Container/BulletContainer.h>


// engine
#include <Engine/Foundation/Math/Vector3.h>

using namespace PlayerShoot;

PlayerShootingController::PlayerShootingController(BulletContainer* container){
	straightShooter_ = std::make_unique<StraightBulletShooter>(container, BulletID::Player_Straight);
	homingShooter_ = std::make_unique<PlayerHomingBulletShooter>(container,BulletID::Player_Homing);
}

bool PlayerShootingController::RequestShoot(const CalyxMath::Vector3& pos, const CalyxMath::Vector3& dir){
	if (shootCooldown_ > 0.0f) return false;

	switch (bulletMode_){
		case BulletMode::Straight: RequestShootStraight(pos, dir); break;
		case BulletMode::Homing: RequestShootHoming(pos, dir); break;
	}

	shootCooldown_ = kShootInterval_;
	return true;
}

void PlayerShootingController::Initialize() {
	straightShooter_ = std::make_unique<StraightBulletShooter>(bulletContainer_.get(), BulletID::Player_Straight);
	homingShooter_ = std::make_unique<PlayerHomingBulletShooter>(bulletContainer_.get(),BulletID::Player_Homing);
}

void PlayerShootingController::Update(float dt){
	bulletContainer_->Update(dt);
	BaseShootingController::Update(dt);
}

void PlayerShootingController::SetTargets(const std::vector<std::shared_ptr<Enemy>>& targets){
	if (homingShooter_) homingShooter_->SetTargets(targets);
}

void PlayerShootingController::SetBulletContainer(std::unique_ptr<BulletContainer> container){
	bulletContainer_ = std::move(container);
}

void PlayerShootingController::RequestShootStraight(const CalyxMath::Vector3& pos, const CalyxMath::Vector3& dir){
	if (straightShooter_) straightShooter_->Shoot(pos, dir);
}

void PlayerShootingController::RequestShootHoming(const CalyxMath::Vector3& pos, const CalyxMath::Vector3& dir){
	if (homingShooter_)   homingShooter_->Shoot(pos, dir);
}

void PlayerShootingController::SetMode(BulletMode bulletMode){
	bulletMode_ = bulletMode;
}