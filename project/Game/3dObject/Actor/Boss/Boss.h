#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
// game
#include <Game/Battle/Shooting/ShootingController/BossShootingController.h>

class Boss final :
	public Actor {
	enum class DeathState { Alive, Dying, Dead };
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	Boss(const std::string& modelName, const std::string objName);
	~Boss()override;

	void Initialize()override;
	void Update(float dt)override;

	//--------- collider -----------------------------------------------------
	void OnCollisionEnter(Collider* other)override;
	void OnCollisionStay([[maybe_unused]] Collider* other)override {}
	void OnCollisionExit([[maybe_unused]] Collider* other)override {}
	
	//--------- accessor -----------------------------------------------------
	const Vector3 GetCenterPos()const override;
	void SetShootingController(std::unique_ptr<BossShootingController>);
	void SetPlayerTransform(const WorldTransform* position);
	
private:
	//===================================================================*/
	//						private methods
	//===================================================================*/
	DeathState deathState_ = DeathState::Alive;
	Vector3 deathRotateAxis_ = { 0, 0, 1 }; // 傾く軸
	Vector3 basePosition_{};		// サイン波の基準位置
	bool isDead_ = false;			// 死んだフラグ
	float deathRotation_ = 0.0f;	// 傾きの進行度
	float waveTime_ = 0.0f;			// 経過時間
	float waveAmplitude_ = 1.0f;	// 振れ幅
	float waveSpeed_ = 2.0f;		// サイン波の速さ
	float deathTimer_ = 0.0f;		// 死亡演出用
	float deathLength_ = 1.5f;		// 倒れ終わるまでの秒数
	
	const WorldTransform* playerTransform_ = nullptr;
	std::unique_ptr<BossShootingController> shootingController_ = nullptr;
};

