#pragma once

// engine
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>

// game
#include <Game/3dObject/Actor/Bullet/BaseBullet.h>

struct SoftHomingParam {
	float turnRateRadPerSec = 2.2f;  // 1秒に回せる角度上限（2.0〜3.0で“軽い”）
	float startDelaySec = 0.20f; // 発射直後は直進→少し経ってから曲げる
	float endTimeSec = 2.0f;  // この時刻以降はホーミング終了（直進化）
	float damping = 0.15f; // 追従の“にゅっ”感（0=即時反映）
	bool  enable = true;
};

class EnemyBullet
	: public BaseBullet {
public:
	//===================================================================*/
	//		public methods
	//===================================================================*/
	EnemyBullet() = default;
	EnemyBullet(const std::string& modelName, const std::string& name);
	~EnemyBullet()override;

	void Initialize()override;
	void OnShot()override;

	void EnableSoftHoming(const SoftHomingParam& p) { homing_ = p; }
	void SetTarget(std::weak_ptr<SceneObject> w) { wTarget_ = std::move(w); }
	void Update(float dt) override;
	//--------- accessor ---------------------------------------------------

private:
	std::shared_ptr<ParticleSystemObject> trailFx_;
	std::shared_ptr<ParticleSystemObject> shootFx_;

	SoftHomingParam homing_{};
	std::weak_ptr<SceneObject> wTarget_{};
	float age_ = 0.0f;

};
