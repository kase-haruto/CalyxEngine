#pragma once

/* =========================================================================
   Include space
   ========================================================================= */
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Game/3dObject/Actor/Bullet/Container/BulletContainer.h>
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Renderer/Sprite/Sprite.h>

class Enemy;

/* =========================================================================
   Player Class
   ========================================================================= */
class Player 
	: public Actor{
public:
	//=====================================================================
	// Public Methods
	//=====================================================================
	Player() = default;
	Player(const std::string& modelName,
		   std::optional<std::string> objectName = std::nullopt);
	virtual ~Player() = default;

	/* mainFunc =========================================================== */
	void Initialize() override;
	void Update() override;
	void Draw(ID3D12GraphicsCommandList* cmdList) override;
	void DerivativeGui() override;

	/* accessor =========================================================== */
	//settter
	void SetParent(const WorldTransform* parent);
	void SetBulletContainer(BulletContainer* bulletContainer);
	void SetEnemyList(const std::list<std::shared_ptr<Enemy>>& targets);

	//getter
	std::vector<Sprite*> GetAllSprites();
	const Vector3 GetCenterPos() const override;

private:
	//=====================================================================
	// Private Methods
	//=====================================================================
	void Move();
	void Shoot();
	void UpdateReticlePosition();
	void UpdateTilt(const Vector3& moveVector);

private:
	//=====================================================================
	// Private Variables
	//=====================================================================

	BulletContainer* bulletContainer_ = nullptr;	//< 弾コンテナへの参照
	float shootInterval_ = 0.3f;					//< 発射間隔
	const float kMaxShootInterval_ = 0.3f;			//< 最大発射間隔
	Vector3 lastMoveVector_;						//< 最後の移動ベクトル

	WorldTransform reticleTransform_;						//< レティクルのワールド変換
	std::array<std::unique_ptr<Sprite>, 4> reticleSprites_;	//< レティクルのスプライト
	std::vector<std::unique_ptr<Sprite>> lifeSprite_;		//< ライフゲージスプライト
	std::unique_ptr<Sprite> attackSprite_;					//< 攻撃状態スプライト

	std::list<std::shared_ptr<Enemy>> targets_;	// 敵の共有ポインタリスト
};
