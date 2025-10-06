#pragma once

/* =========================================================================
   Include space
   ========================================================================= */
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Game/3dObject/Actor/Bullet/Container/BulletContainer.h>
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/Scene/Runtime/IRuntimeBehaviour.h>
// game
#include <Game/3dObject/Actor/Enemy/Enemy.h>
#include <Game/Battle/Shooting/ShootingController/PlayerShootingController.h>
#include <Game/Input/PlayerInput/PlayerInputHandler.h>

class PlayerDodge;

/* =========================================================================
   Player Class
   ========================================================================= */
class Player :
	public Actor,
	public IRuntimeBehaviour {
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
	void RefreshLifeUI();
	void Update(float dt) override;
	void Draw(ID3D12GraphicsCommandList* cmdList) override;
	void DerivativeGui() override;
	void MoveBy(const Vector3& delta);
	void MoveReticle(const Vector3& offset);
	void RequestShoot();
	void RequestLockOn();

	void RequestLockOnTargetClear();
	std::string_view GetTypeName() const override { return "Player"; }

	/* runtime ==============================================================*/
	void Start() override;

	void OnCollisionEnter(Collider* other)override;
	void OnCollisionStay([[maybe_unused]] Collider* other)override {}
	void OnCollisionExit([[maybe_unused]] Collider* other)override {}

	/* accessor =========================================================== */
	//settter
	void SetParent(WorldTransform* parent);
	void SetEnemyList(const std::list<std::shared_ptr<Enemy>>& list) {
		targets_.assign(list.begin(), list.end());
	}
	void SetShootingController(std::unique_ptr<PlayerShootingController> sc);
	void SetInputHandler(std::unique_ptr<PlayerInputHandler> ih);

	//getter
	std::vector<Sprite*> GetAllSprites();
	const Vector3 GetCenterPos() const override;
	float GetMoveSpeed() const { return moveSpeed_; }
	std::optional<float> GetShootCooldown();
	std::optional<const float> GetMaxShootInterval() const;
	const std::vector<std::shared_ptr<Enemy>>& GetLockedOnTargets() const { return lockedOnTargets_; }

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
	std::unique_ptr<PlayerShootingController>shootingController_;
	std::unique_ptr<PlayerInputHandler> inputHandler_ = nullptr;
	std::unique_ptr<PlayerDodge> dodge_;					//< ジャスト回避

	Vector3 lastMoveVector_;								//< 最後の移動ベクトル
	WorldTransform reticleTransform_;						//< レティクルのワールド変換


	std::vector<std::shared_ptr<Enemy>> targets_;
	std::vector<std::shared_ptr<Enemy>> lockedOnTargets_;

	// sprites
	std::array<std::unique_ptr<Sprite>, 4> reticleSprites_;	//< レティクルのスプライト
	std::vector<std::unique_ptr<Sprite>> lifeSprite_;		//< ライフゲージスプライト
	std::vector<std::unique_ptr<Sprite>> lockOnSprites_;	//< ロックオンマーカー
};
