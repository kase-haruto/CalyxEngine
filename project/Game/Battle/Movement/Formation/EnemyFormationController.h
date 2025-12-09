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
	Count,
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

	float dissolveTime = 0.0f;

	DissolvePattern dissolvePattern = DissolvePattern::FourWay;
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

inline void to_json(nlohmann::json& j, const EnemyFormationConfig& f)
{
	j = {
		{"useFormation", f.useFormation},
		{"motionType", (int)f.motionType},
		{"baseZ", f.baseZ},
		{"speedZ", f.speedZ},
		{"radius", f.radius},
		{"angularSpeed", f.angularSpeed},
		{"snakeAmpX", f.snakeAmpX},
		{"snakeAmpY", f.snakeAmpY},
		{"snakeFreqX", f.snakeFreqX},
		{"snakeFreqY", f.snakeFreqY},
		{"dissolveTime", f.dissolveTime},
		{"dissolvePattern", (int)f.dissolvePattern}
	};
}

inline void from_json(const nlohmann::json& j, EnemyFormationConfig& f)
{
	if(j.contains("useFormation")) j.at("useFormation").get_to(f.useFormation);

	int mt = 0;
	if(j.contains("motionType")) {
		j.at("motionType").get_to(mt);
		f.motionType = (EnemyFormationMotionType)mt;
	}

	if(j.contains("baseZ"))          j.at("baseZ").get_to(f.baseZ);
	if(j.contains("speedZ"))         j.at("speedZ").get_to(f.speedZ);
	if(j.contains("radius"))         j.at("radius").get_to(f.radius);
	if(j.contains("angularSpeed"))   j.at("angularSpeed").get_to(f.angularSpeed);

	if(j.contains("snakeAmpX"))      j.at("snakeAmpX").get_to(f.snakeAmpX);
	if(j.contains("snakeAmpY"))      j.at("snakeAmpY").get_to(f.snakeAmpY);
	if(j.contains("snakeFreqX"))     j.at("snakeFreqX").get_to(f.snakeFreqX);
	if(j.contains("snakeFreqY"))     j.at("snakeFreqY").get_to(f.snakeFreqY);

	if(j.contains("dissolveTime"))   j.at("dissolveTime").get_to(f.dissolveTime);

	int dp = 0;
	if(j.contains("dissolvePattern")) {
		j.at("dissolvePattern").get_to(dp);
		f.dissolvePattern = (DissolvePattern)dp;
	}
}