#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Game/3dObject/Actor/Bullet/Container/BulletContainer.h>
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Renderer/Sprite/Sprite.h>

class Enemy;

/* ========================================================================
/* Player
/* ===================================================================== */
class Player :
	public Actor {
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	Player() = default;
	Player(const std::string& modelName,
		   std::optional<std::string> objectName = std::nullopt);
	virtual ~Player() = default;

	void Initialize()override;
	void Update()override;
	void Draw(ID3D12GraphicsCommandList* cmdList)override;

	/* ui =========================================*/
	void DerivativeGui()override;

	void SetParent(const WorldTransform* parent) {
		worldTransform_.parent = parent;
	}

	void SetBulletContainer(BulletContainer* bulletContainer) {
		bulletContainer_ = bulletContainer;
	}

	std::vector<Sprite*> GetAllSprites();
	void SetEnemyList(const std::list<std::shared_ptr<Enemy>>& targets){
		targets_ = targets;
	}
	const Vector3 GetCenterPos()const override;
private:
	//===================================================================*/
	//                   private methods
	//===================================================================*/
	void Move();
	void Shoot();
	void UpdateReticlePosition();
	void UpdateTilt(const Vector3& moveVector);
	float EaseForwardThenReturn(float t);
	void InitializeEffect();



private:
	//===================================================================*/
	//                   private variables
	//===================================================================*/
	BulletContainer* bulletContainer_ = nullptr;	// 弾コンテナ
	float shootInterval_ = 0.3f;	// 発射間隔
	const float kMaxShootInterval_ = 0.3f;	// 最大発射間隔
	Vector3 lastMoveVector_;
	// ローリング関連
	WorldTransform reticleTransform_;
	std::array<std::unique_ptr<Sprite>,4> reticleSprites_;
	std::vector < std::unique_ptr<Sprite>> lifeSprite_;
	std::unique_ptr<Sprite> attackSprite_;

	std::list<std::shared_ptr<Enemy>> targets_;

};
