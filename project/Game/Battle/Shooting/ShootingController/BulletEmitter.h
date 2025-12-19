#pragma once
#include <memory>
#include <string>
#include <Engine/Foundation/Math/Vector3.h>
#include "Game/Battle/Shooting/Details/FireScheduler.h"
#include "Game/Battle/Shooting/Details/AimProvider.h"
#include "Game/Battle/Shooting/Pattern/IShootPattern.h"
#include <Game/Battle/Shooting/Details/IShootSink.h>

struct BulletEmitterConfig{
	float shotSpeed = 0.0f;   // 不要なら0
	std::string tag;          // 任意の識別
};
struct BulletEmitterContext{
	CalyxMath::Vector3 origin {};
	CalyxMath::Vector3 selfForward {0,0,1};
	CalyxMath::Vector3 targetPos {};
	CalyxMath::Vector3 targetVel {};
};

class BulletEmitter{
public:
	BulletEmitter(BulletEmitterConfig cfg,
				  std::unique_ptr<IShootSink> sink,
				  std::unique_ptr<IAimProvider> aim,
				  IShootPattern* pattern,
				  FireScheduler scheduler)
		: cfg_(cfg), sink_(std::move(sink)), aim_(std::move(aim)),
		pattern_(pattern), scheduler_(scheduler){}

	void Update(float dt, const BulletEmitterContext& ctx);

	void SetPattern(IShootPattern* p);

	FireScheduler& Scheduler(){ return scheduler_; }
	IShootPattern* Pattern(){ return pattern_; }
	IAimProvider* Aim(){ return aim_.get(); }

private:
	BulletEmitterConfig cfg_;
	std::unique_ptr<IShootSink>   sink_;
	std::unique_ptr<IAimProvider> aim_;
	IShootPattern* pattern_ = nullptr;               // 非所有参照
	std::unique_ptr<IShootPattern> ownedPattern_;
	FireScheduler                 scheduler_;
};