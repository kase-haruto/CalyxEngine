#pragma once

// game
#include "Engine/Application/Effects/FxObject.h"

#include <Game/3dObject/Actor/Bullet/BaseBullet.h>

struct SoftHomingParam {
	float turnRateRadPerSec = 2.2f;		// 1秒に回せる角度上限（2.0〜3.0で“軽い”）
	float startDelaySec = 0.20f;		// 発射直後は直進→少し経ってから曲げる
	float endTimeSec = 2.0f;			// この時刻以降はホーミング終了（直進化）
	float damping = 0.15f;				// 追従の“にゅっ”感（0=即時反映）
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
	void Update(float dt) override;
	void OnShot()override;

	//-- accessor --------------------------------------------------------//
	void EnableSoftHoming(const SoftHomingParam& p) { homing_ = p; }
	void SetTarget(std::weak_ptr<SceneObject> w) { wTarget_ = std::move(w); }

private:
	//===================================================================*/
	//		private methods
	//===================================================================*/
	void direct2Camera();
	
private:
	SoftHomingParam homing_{};					//< ホーミングパラメータ
	std::weak_ptr<SceneObject> wTarget_{};		//< ターゲットのポインタ
	std::weak_ptr<CalyxEffect::FxObject> trailFx_{};				 //< トレイルエフェクト
	float age_ = 0.0f;							//< 寿命
};