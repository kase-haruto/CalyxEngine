#pragma once

/* =========================================================================
   Include space
   ========================================================================= */
// engine
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/Scene/Runtime/IRuntimeBehaviour.h>

// game
#include "LockOn/PlayerLockOn.h"
#include "Move/PlayerMoveController.h"
#include <Game/2d/HpGauge.h>
#include <Game/3dObject/Actor/Bullet/Container/BulletContainer.h>
#include <Game/3dObject/Actor/Enemy/Enemy.h>
#include <Game/Battle/Shooting/ShootingController/PlayerShootingController.h>

// fwd
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
	/**
	 * \brief 初期化
	 */
	void Initialize() override;
	/**
	 * \brief 更新
	 * \param dt デルタタイム
	 */
	void Update(float dt) override;
	/**
	 * \brief 描画
	 * \param cmdList コマンドリスト
	 */
	void Draw(ID3D12GraphicsCommandList* cmdList) override;
	/**
	 * \brief デリバティブGUI
	 */
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
	void RequestShoot() const;
	/**
	 * \brief ロックオンをリクエストする
	 */
	void RequestLockOn() const;
	/**
	 * \brief 移動ベクトルに応じて傾きを更新する
	 * \param moveVector 移動ベクトル
	 */
	void UpdateTilt(const Vector3& moveVector);
	/**
	 * \brief 危険察知ソースをアタッチする
	 * \param dir 敵ディレクトリ
	 */
	void AttachDangerSenseSource(EnemyDirectory* dir) const;
	/**
	 * \brief ロックオン解除をリクエストする
	 */
	void RequestLockOnTargetClear() const;

	/* runtime ==============================================================*/
	/**
	 * \brief ランタイムスタート処理
	 */
	void Start() override;
	void RequestDodge() const;

	void OnCollisionEnter(Collider* other) override;
	void OnCollisionStay([[maybe_unused]] Collider* other) override {}
	void OnCollisionExit([[maybe_unused]] Collider* other) override {}

	/* accessor =========================================================== */
	// setter
	void SetParent(WorldTransform* parent);
	void AttachEnemyList(const std::list<std::shared_ptr<Enemy>>& list) const;
	void SetShootingController(std::unique_ptr<PlayerShootingController> sc);
	void SetInputHandler(std::unique_ptr<PlayerInputHandler> ih);

	// getter
	std::string_view           GetTypeName() const override { return "Player"; }
	PlayerDangerSense*         GetDangerSense() const { return danger_.get(); }
	std::vector<Sprite*>       GetAllSprites() const;
	const Vector3              GetCenterPos() const override;
	std::optional<float>       GetShootCooldown() const;
	std::optional<const float> GetMaxShootInterval() const;
	Vector3                    GetReticleWorldPos() const { return reticleTransform_.GetWorldPosition(); }

private:
	//=====================================================================
	// Private Methods
	//=====================================================================
	/**
	 * \brief レティクルのポジションを更新
	 */
	void UpdateReticlePosition();

private:
	//=====================================================================
	// Private Variables
	//=====================================================================
	PlayerMoveController                      moveCtrler_;                   //< 移動コントローラ
	std::unique_ptr<PlayerDodgeMotion>        dodgeMotion_        = nullptr; //< 回避モーション
	std::unique_ptr<PlayerDodgeSystem>        dodgeSystem_        = nullptr; //< 回避システム
	std::unique_ptr<PlayerInputHandler>       inputHandler_       = nullptr; //< 入力ハンドラ
	std::unique_ptr<PlayerDangerSense>        danger_             = nullptr; //< 危機察知
	std::unique_ptr<PlayerDamageHandler>      damageHandler_      = nullptr; //< ダメージハンドラ
	std::unique_ptr<PlayerLockOn>             lockOn_             = nullptr; //< ロックオンシステム
	std::unique_ptr<PlayerShootingController> shootingController_ = nullptr; //< 射撃コントローラ

	Vector3        lastMoveVector_;   //< 最後の移動ベクトル
	WorldTransform reticleTransform_; //< レティクルのワールド変換

	// sprites
	std::array<std::unique_ptr<Sprite>,4> reticleSprites_; //< レティクルのスプライト
	std::unique_ptr<HpGauge>              hpGauge_;        //< HPゲージ

	// --- Auto Lock-On params ---
	bool autoLockOn_ = true; // オートロックオン有効/無効

	// 画面内クランプ用設定
	bool  clampPlayerInView_  = true;
	bool  clampReticleInView_ = true;
	float clampMarginXpx_     = 24.0f; // 左右の余白(px)
	float clampMarginYpx_     = 24.0f; // 上下の余白(px)

	// effect
	std::shared_ptr<FxObject> shootFx_;
};