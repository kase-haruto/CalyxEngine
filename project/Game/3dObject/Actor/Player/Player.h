#pragma once

/* =========================================================================
   Include space
   ========================================================================= */
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Game/3dObject/Actor/Bullet/Container/BulletContainer.h>
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/Scene/Runtime/IRuntimeBehaviour.h>
#include <Engine/Application/Effects/FxObject.h>

// game
#include "Move/PlayerMoveController.h"

#include <Game/3dObject/Actor/Enemy/Enemy.h>
#include <Game/Battle/Shooting/ShootingController/PlayerShootingController.h>
#include <Game/2d/HpGauge.h>

class PlayerInputHandler;
class PlayerDodgeSystem;
class EnemyDirectory;
class PlayerDangerSense;
class PlayerDodge;
class PlayerDodgeMotion;
class PlayerDamageHandler;

/**
 * \brief
 * 操作するキャラクタークラス
 */
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
	virtual ~Player() override;

	/* mainFunc =========================================================== */
	void Initialize() override;
	void Update(float dt) override;
	void Draw(ID3D12GraphicsCommandList* cmdList) override;
	void DerivativeGui() override;

	/**
	 * \brief 移動量の追加をRequest
	 * \param delta
	 */
	void AddMoveRequest(const Vector3& delta);
	/**
	 * \brief レティクルをオフセット分移動する
	 * \param offset
	 */
	void MoveReticle(const Vector3& offset);
	/**
	 * \brief 弾の発射をリクエストする
	 */
	void RequestShoot();
	/**
	 * \brief ロックオンをリクエストする
	 */
	void RequestLockOn();
	/**
	 * \brief 移動ベクトルに応じて傾きを更新する
	 * \param moveVector 移動ベクトル
	 */
	void UpdateTilt(const Vector3& moveVector);
	/**
	 * \brief 危険察知ソースをアタッチする
	 * \param dir 敵ディレクトリ
	 */
	void AttachDangerSenseSource(EnemyDirectory* dir);
	/**
	 * \brief ロックオン解除をリクエストする
	 */
	void RequestLockOnTargetClear();

	/* runtime ==============================================================*/
	/**
	 * \brief ランタイムスタート処理
	 */
	void Start() override;
	void RequestDodge();

	void OnCollisionEnter(Collider* other) override;
	void OnCollisionStay([[maybe_unused]] Collider* other) override {}
	void OnCollisionExit([[maybe_unused]] Collider* other) override {}

	/* accessor =========================================================== */
	//setter
	void SetParent(WorldTransform* parent);
	void SetEnemyList(const std::list<std::shared_ptr<Enemy>>& list) { targets_.assign(list.begin(),list.end()); }
	void SetShootingController(std::unique_ptr<PlayerShootingController> sc);
	void SetInputHandler(std::unique_ptr<PlayerInputHandler> ih);

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
	/**
	 * \brief レティクルのポジションを更新
	 */
	void UpdateReticlePosition();
	/**
	 * \brief autoロックオン処理
	 * \param dt デルタタイム
	 */
	void UpdateAutoLockOn(float dt);
	/**
	 * \brief ロックオンマーカーを取得する
	 * \return
	 */
	std::unique_ptr<Sprite> AcquireMarker();
	/**
	 * \brief ロックオンmarkerを再利用プールに戻す
	 * \param s スプライトポインタ
	 */
	void RecycleMarker(std::unique_ptr<Sprite> s);
	/**
	 * \brief
	 * \param n
	 */
	void PrewarmLockMarkers(size_t n);
	/**
	 * \brief 死んだ敵のロックオン解除
	 */
	void PurgeDeadLockedTargets();

private:
	//=====================================================================
	// Private Variables
	//=====================================================================
	PlayerMoveController                      moveCtrler_;
	std::unique_ptr<PlayerDodgeMotion>        dodgeMotion_; //< 回避モーション
	std::unique_ptr<PlayerDodgeSystem>        dodgeSystem_ = nullptr;
	std::unique_ptr<PlayerShootingController> shootingController_;
	std::unique_ptr<PlayerInputHandler>       inputHandler_ = nullptr;
	std::unique_ptr<PlayerDangerSense>        danger_; //< 危機察知
	std::unique_ptr<PlayerDamageHandler>      damageHandler_ = nullptr;

	Vector3        lastMoveVector_;   //< 最後の移動ベクトル
	WorldTransform reticleTransform_; //< レティクルのワールド変換


	std::vector<std::shared_ptr<Enemy>> targets_;
	std::vector<std::shared_ptr<Enemy>> lockedOnTargets_;

	// sprites
	std::array<std::unique_ptr<Sprite>,4> reticleSprites_; //< レティクルのスプライト
	std::unique_ptr<HpGauge>              hpGauge_;        //< HPゲージ
	std::vector<std::unique_ptr<Sprite>>  markerPool_;     // 未使用(再利用待ち)のマーカー
	std::vector<std::unique_ptr<Sprite>>  lockOnSprites_;  // 未使用(再利用待ち)のマーカー

	// ロックオン
	float  lockOnRadiusPx_ = 60.0f; //
	size_t maxLockOn_      = 5;     //  kMaxLockOn

	// --- Auto Lock-On params ---
	bool  autoLockOn_            = true; // オートロックオン有効/無効
	float lockOnAcquireRadiusPx_ = 60.0f;
	float lockOnReleaseRadiusPx_ = 150.0f; // 解除半径
	float lockOnRefreshInterval_ = 0.15f;  // 判定間隔（秒）
	float lockOnRefreshTimer_    = 0.0f;

	// 画面内クランプ用設定
	bool  clampPlayerInView_  = true;
	bool  clampReticleInView_ = true;
	float clampMarginXpx_     = 24.0f; // 左右の余白(px)
	float clampMarginYpx_     = 24.0f; // 上下の余白(px)

	// effect
	std::shared_ptr<FxObject> shootFx_;
};