#pragma once

/* =========================================================================
   Include space
   ========================================================================= */
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Game/3dObject/Actor/Bullet/Container/BulletContainer.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/Scene/Runtime/IRuntimeBehaviour.h>
// game
#include <Game/3dObject/Actor/Enemy/Enemy.h>
#include <Game/Battle/Shooting/ShootingController/PlayerShootingController.h>
#include <Game/Input/PlayerInput/PlayerInputHandler.h>

class EnemyDirectory;
class PlayerDangerSense;
class PlayerDodge;
class PlayerDodgeMotion;

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
	Player();
	Player(const std::string&         modelName,
		   std::optional<std::string> objectName = std::nullopt);
	virtual ~Player();

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

	void AttachDangerSenseSource(EnemyDirectory* dir);
	void RequestLockOnTargetClear();

	/* runtime ==============================================================*/
	void Start() override;

	void OnCollisionEnter(Collider* other) override;
	void OnCollisionStay([[maybe_unused]] Collider* other) override {}
	void OnCollisionExit([[maybe_unused]] Collider* other) override {}

	/* accessor =========================================================== */
	//settter
	void SetParent(WorldTransform* parent);
	void SetEnemyList(const std::list<std::shared_ptr<Enemy>>& list) { targets_.assign(list.begin(),list.end()); }
	void SetShootingController(std::unique_ptr<PlayerShootingController> sc);
	void SetInputHandler(std::unique_ptr<PlayerInputHandler> ih);
	void SetInvincibleFor(float seconds); // 指定秒数だけ無敵にする（重ねがけは長い方を優先）
	bool CanBeDamaged() const { return !IsInvincible(); }

	//getter
	std::string_view                           GetTypeName() const override { return "Player"; }
	PlayerDangerSense*                         GetDangerSense() { return danger_.get(); }
	std::vector<Sprite*>                       GetAllSprites();
	const Vector3                              GetCenterPos() const override;
	float                                      GetMoveSpeed() const { return moveSpeed_; }
	std::optional<float>                       GetShootCooldown();
	std::optional<const float>                 GetMaxShootInterval() const;
	const std::vector<std::shared_ptr<Enemy>>& GetLockedOnTargets() const { return lockedOnTargets_; }

private:
	//=====================================================================
	// Private Methods
	//=====================================================================
	void Move();
	void Shoot();
	void UpdateReticlePosition();
	void UpdateTilt(const Vector3& moveVector);
	void UpdateAutoLockOn(float dt);

	// 取得/返却/初期確保
	// ロックオン
	std::unique_ptr<Sprite> AcquireMarker();
	void RecycleMarker(std::unique_ptr<Sprite> s);
	void PrewarmLockMarkers(size_t n);

	// 死んだ敵のロックオンを外す
	void PurgeDeadLockedTargets();

	// === 無敵API ===
	bool IsInvincible() const;
	void UpdateInvincibility(float dt);

private:
	//=====================================================================
	// Private Variables
	//=====================================================================
	std::unique_ptr<PlayerShootingController> shootingController_;
	std::unique_ptr<PlayerInputHandler>       inputHandler_ = nullptr;
	std::unique_ptr<PlayerDodge>              dodge_;       //< ジャスト回避
	std::unique_ptr<PlayerDangerSense>        danger_;      //< 危機察知
	std::unique_ptr<PlayerDodgeMotion>        dodgeMotion_; //< 回避モーション

	Vector3        lastMoveVector_;   //< 最後の移動ベクトル
	WorldTransform reticleTransform_; //< レティクルのワールド変換


	std::vector<std::shared_ptr<Enemy>> targets_;
	std::vector<std::shared_ptr<Enemy>> lockedOnTargets_;

	// sprites
	std::array<std::unique_ptr<Sprite>,4> reticleSprites_; //< レティクルのスプライト
	std::vector<std::unique_ptr<Sprite>>  lifeSprite_;     //< ライフゲージスプライト
	std::vector<std::unique_ptr<Sprite>> markerPool_; // 未使用(再利用待ち)のマーカー
	std::vector<std::unique_ptr<Sprite>> lockOnSprites_; // 未使用(再利用待ち)のマーカー

	// ロックオン
	float  lockOnRadiusPx_ = 60.0f; //
	size_t maxLockOn_      = 5;     //  kMaxLockOn

	// --- Auto Lock-On params ---
	bool  autoLockOn_            = true;   // オートロックオン有効/無効
	float lockOnAcquireRadiusPx_ = 60.0f;
	float lockOnReleaseRadiusPx_ = 150.0f; // 解除半径
	float lockOnRefreshInterval_ = 0.15f;  // 判定間隔（秒）
	float lockOnRefreshTimer_    = 0.0f;


	// 無敵時かん用
	float invincibleTimer_ = 0.0f; // >0 の間は無敵
	// 見た目に使いたければトグル点滅など
	float invincibleBlinkAccum_ = 0.0f;
	bool  invincibleBlinkState_ = true;

	// 画面内クランプ用設定
	bool  clampPlayerInView_  = true;
	bool  clampReticleInView_ = true;
	float clampMarginXpx_     = 24.0f; // 左右の余白(px)
	float clampMarginYpx_     = 24.0f; // 上下の余白(px)
};