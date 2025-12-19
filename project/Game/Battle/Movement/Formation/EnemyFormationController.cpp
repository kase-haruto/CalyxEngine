#include "EnemyFormationController.h"
#include <cmath>

EnemyFormationController::EnemyFormationController() = default;

void EnemyFormationController::Initialize(const EnemyFormationConfig& cfg) {
	cfg_ = cfg;
	time_ = 0.0f;
	pos_  = {0, 0, cfg_.baseZ};
	dissolved_ = false;

	dissolvePattern_ = cfg_.dissolvePattern;

	motionFunc_ = nullptr;

	switch(cfg_.motionType) {
	case EnemyFormationMotionType::Straight:
		motionFunc_ = [c = cfg_](float t) {
			return CalyxMath::Vector3{0.0f, 0.0f, c.baseZ - c.speedZ * t};
	};
		break;

	case EnemyFormationMotionType::Snake:
		motionFunc_ = [c = cfg_](float t) {
			float x = std::sin(t * c.snakeFreqX) * c.snakeAmpX;
			float y = std::sin(t * c.snakeFreqY) * c.snakeAmpY;
			float z = c.baseZ - c.speedZ * t;
			return CalyxMath::Vector3{x, y, z};
	};
		break;

	case EnemyFormationMotionType::Circle:
		motionFunc_ = [c = cfg_](float t) {
			float theta = t * c.angularSpeed;
			float x = std::cos(theta) * c.radius;
			float y = std::sin(theta) * c.radius;
			float z = c.baseZ - c.speedZ * t;
			return CalyxMath::Vector3{x, y, z};
	};
		break;

	default:
		motionFunc_ = [c = cfg_](float t) {
			return CalyxMath::Vector3{0.0f, 0.0f, c.baseZ - c.speedZ * t};
	};
		break;
	}
}

void EnemyFormationController::Update(float dt) {
	time_ += dt;

	if (!dissolved_ && cfg_.dissolveTime > 0.0f && time_ >= cfg_.dissolveTime) {
		Dissolve();
	}

	// 解散後はフォーメーション位置を更新しない
	if (dissolved_) return;

	pos_ = motionFunc_(time_);
}

void EnemyFormationController::Dissolve() {
	if (dissolved_) return;
	dissolved_ = true;
}