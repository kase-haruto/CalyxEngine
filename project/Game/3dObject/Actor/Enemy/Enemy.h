#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/objects/Collider/SphereCollider.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
// game
#include <Game/Battle/Shooting/ShootingController/EnemyShootingController.h>



/* ========================================================================
/* enemy
/* ===================================================================== */
class Enemy :
    public Actor{
	enum class DeathState{ Alive, Dying, Dead };
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	Enemy() = default;
	Enemy(const std::string& modelName,const std::string objName);

	virtual ~Enemy()override;

	void Initialize()override;
	void Update(float dt)override;

	void OnCollisionEnter(Collider* other)override;
	void OnCollisionStay([[maybe_unused]]Collider* other)override {}
	void OnCollisionExit([[maybe_unused]] Collider* other)override {}
	const Vector3 GetCenterPos()const override;
	void SetPosition(const Vector3& position){
		worldTransform_.translation = position;
	};

	void SetParent(WorldTransform* parent);
	void SetShootingController(std::unique_ptr<EnemyShootingController>);
	void SetPlayerTransform(const WorldTransform* position);

	void SetGameplayEngaged(bool v) { gameplayEngaged_ = v; }
	bool IsGameplayEngaged() const { return gameplayEngaged_; }
protected:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	void Move();
	void Shoot();

private:
	//===================================================================*/
	//					private variables
	//===================================================================*/
	DeathState deathState_ = DeathState::Alive;
	Vector3 deathRotateAxis_ = {0, 0, 1}; // 傾く軸
	Vector3 basePosition_{};		// サイン波の基準位置
	const WorldTransform* playerTransform_;
	bool isHit_ = false;			//< 衝突フラグ
	bool isDead_ = false;			//< 死んだフラグ
	bool gameplayEngaged_ = false;	//< 
	float deathRotation_ = 0.0f;	//< 傾きの進行度
	float waveTime_ = 0.0f;			//< 経過時間
	float waveAmplitude_ = 1.0f;	//< 振れ幅
	float waveSpeed_ = 2.0f;		//< サイン波の速さ
	float deathTimer_ = 0.0f;		//< 死亡演出用
	float deathLength_ = 1.5f;		//< 倒れ終わるまでの秒数

	std::unique_ptr<EnemyShootingController> shootingController_ = nullptr;
	std::shared_ptr<ParticleSystemObject> hitFx_;
	std::shared_ptr<ParticleSystemObject> explosionFx_;
};