#pragma once
#include <functional>
#include <Engine/Foundation/Math/Vector3.h>

enum class DissolvePattern {
	AlternatingLeftRight,
	FourWay,
	VShape,
	Circle,
	StraightBack,
};

enum class EnemyFormationMotionType {
	Straight,
	Snake,
	Circle,
};

struct EnemyFormationConfig {
	bool useFormation = true;

	EnemyFormationMotionType motionType = EnemyFormationMotionType::Straight;

	float baseZ  = -80.0f;
	float speedZ = 20.0f;

	float radius       = 30.0f;
	float angularSpeed = 1.0f;

	float snakeAmpX  = 25.0f;
	float snakeAmpY  = 10.0f;
	float snakeFreqX = 2.0f;
	float snakeFreqY = 1.7f;

	// 解散までの時間（秒）
	float dissolveTime = 0.0f;
};

/// --------------------------------------------------------------
/// 隊列（Formation）制御本体
/// --------------------------------------------------------------
class EnemyFormationController {
public:
	using MotionFunc = std::function<Vector3(float)>;

	EnemyFormationController();

	void Initialize(const EnemyFormationConfig& cfg);
	void Update(float dt);

	void Dissolve();

	const Vector3& GetPosition() const { return pos_; }

	float GetTime() const { return time_; }

	const EnemyFormationConfig& GetConfig() const { return cfg_; }

	void SetDissolvePattern(DissolvePattern p) { dissolvePattern_ = p; }
	DissolvePattern GetDissolvePattern() const { return dissolvePattern_; }

	bool IsDissolved() const { return dissolved_; }

private:
	EnemyFormationConfig cfg_;
	MotionFunc motionFunc_;

	float time_ = 0.0f;
	Vector3 pos_ = {0, 0, 0};

	bool dissolved_ = false;

	DissolvePattern dissolvePattern_ = DissolvePattern::FourWay;
};
