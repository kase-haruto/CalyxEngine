#include "EnemyFormationController.h"
#include <cmath>

EnemyFormationController::EnemyFormationController() = default;

void EnemyFormationController::Initialize(const EnemyFormationConfig& cfg) {
	cfg_		= cfg;
	time_		= 0.0f;
	pos_		= {0, 0, cfg_.baseZ};
	motionFunc_ = nullptr;

	switch(cfg_.motionType) {
	case EnemyFormationMotionType::Straight: {
		// Z マイナス方向へ等速直進
		motionFunc_ = [c = cfg_](float t) -> Vector3 {
			return Vector3{0.0f, 0.0f, c.baseZ - c.speedZ * t};
		};
		break;
	}
	case EnemyFormationMotionType::Snake: {
		motionFunc_ = [c = cfg_](float t) -> Vector3 {
			float x = std::sin(t * c.snakeFreqX) * c.snakeAmpX;
			float y = std::sin(t * c.snakeFreqY) * c.snakeAmpY;
			float z = c.baseZ - c.speedZ * t;
			return Vector3{x, y, z};
		};
		break;
	}
	case EnemyFormationMotionType::Circle: {
		motionFunc_ = [c = cfg_](float t) -> Vector3 {
			// XY 平面で円運動しながら Z 方向へ進む
			float theta = t * c.angularSpeed;
			float x		= std::cos(theta) * c.radius;
			float y		= std::sin(theta) * c.radius;
			float z		= c.baseZ - c.speedZ * t;
			return Vector3{x, y, z};
		};
		break;
	}
	default:
		// 一応 Straight と同じ
		motionFunc_ = [c = cfg_](float t) -> Vector3 {
			return Vector3{0.0f, 0.0f, c.baseZ - c.speedZ * t};
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