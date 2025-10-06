#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/objects/Collider/SphereCollider.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineData.h>

// game
#include <Game/Battle/Shooting/ShootingController/EnemyShootingController.h>
#include <Game/Battle/Shooting/Details/IShootSink.h>
#include <Game/Battle/Shooting/Details/FireScheduler.h>
#include <Game/Battle/Shooting/ShootingController/BulletEmitter.h>
#include <Game/Battle/Movement/FollowSpline/SplineFollower.h>

/* ========================================================================
/* enemy
/* ===================================================================== */
class Enemy 
	: public Actor {
	enum class DeathState { Alive, Dying, Dead };
public:
	//===================================================================*/
	//                      public methods
	//===================================================================*/
	Enemy() = default;
	Enemy(const std::string& modelName, const std::string objName);
	virtual ~Enemy() override;

	void Initialize() override;
	void Update(float dt) override;

	// collision ---------------------------------------------------------//
	void OnCollisionEnter(Collider* other) override;
	void OnCollisionStay([[maybe_unused]] Collider* other) override {}
	void OnCollisionExit([[maybe_unused]] Collider* other) override {}
	
	// accessor ---------------------------------------------------------//
	void SetPosition(const Vector3& position) { worldTransform_.translation = position; };
	void SetParent(WorldTransform* parent);
	void SetShootingController(std::unique_ptr<EnemyShootingController>);
	void SetPlayerTransform(const WorldTransform* position);
	void SetRouteSpline(const SplineData& data);

	void BindPath(const SplineData* path, float startT = 0.0f, bool loop = true, int arcSamplesPerSeg = 48) {
		mover_.BindPath(path, startT, loop, arcSamplesPerSeg);
	}
	void SetSpawnerAnchor(const WorldTransform* spawnerTf) {
		mover_.SetAnchor(spawnerTf, /*inheritPos=*/true, /*inheritRot=*/true);
	}
	void SetPathWorldSpeed(float mps) { mover_.SetWorldSpeed(mps); }
	void SetPathLookMode(SplineFollower::LookMode m) { mover_.SetLookMode(m); }
	void SetPathYOffset(float y) { mover_.SetYOffset(y); }
	void SetPathTarget(const WorldTransform* t) { mover_.SetTargetTransform(t); }
	bool PathFinished() const { return mover_.IsFinished(); }

	const Vector3 GetCenterPos() const override;
	void SetGameplayEngaged(bool v) { gameplayEngaged_ = v; }
	bool IsGameplayEngaged() const { return gameplayEngaged_; }

	BulletContainer* GetBulletContainer();
	const BulletContainer* GetBulletContainer() const;


protected:
	//===================================================================*/
	//                      protected methods
	//===================================================================*/
	void Move();
	void Shoot();

private:
	//===================================================================*/
	//                      private methods
	//===================================================================*/
	void BuildEmitterIfReady();

private:
	//===================================================================*/
	//                      private variables
	//===================================================================*/
	const WorldTransform* playerTransform_ = nullptr;
	DeathState deathState_ = DeathState::Alive;
	SplineFollower mover_;					//< 移動
	SplineData moveRoute_{};				//< 経路データ
	Vector3 deathRotateAxis_ = { 0, 0, 1 };	//< 傾く軸
	Vector3 basePosition_{};				//< サイン波の基準位置

	bool hasRoute_ = false;					//< 移動ルートを取得済みか
	bool isHit_ = false;					//< 衝突フラグ
	bool isDead_ = false;					//< 死んだフラグ
	bool gameplayEngaged_ = false;			//<
	float deathRotation_ = 0.0f;			//< 傾きの進行度
	float waveTime_ = 0.0f;					//< 経過時間
	float waveAmplitude_ = 1.0f;			//< 振れ幅
	float waveSpeed_ = 2.0f;				//< サイン波の速さ
	float deathTimer_ = 0.0f;				//< 死亡演出用
	float deathLength_ = 1.5f;				//< 倒れ終わるまでの秒数

	std::unique_ptr<BulletEmitter> emitter_;                       //< 一度だけ生成して保持
	std::unique_ptr<EnemyShootingController> shootingController_;  //< 下流コントローラ
	std::shared_ptr<ParticleSystemObject> hitFx_;
	std::shared_ptr<ParticleSystemObject> explosionFx_;
};
