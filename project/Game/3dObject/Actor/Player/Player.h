#pragma once

/* =========================================================================
   Include space
   ========================================================================= */
// engine
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

// game
#include "Context/PlayerContext.h"
#include "Dodge/PlayerDodgeMotion.h"
#include "Input/PlayerInput.h"
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
class PlayerDamageHandler;

/*-----------------------------------------------------------------------------------------
 * Player
 * - 操作するキャラクタークラス
 * - 移動、射撃、ロックオン、回避などのアクションを管理
 *---------------------------------------------------------------------------------------*/
class Player
	: public Actor,
	 public CalyxEngine::SerializableObject {
public:
	//=====================================================================
	// Public Methods
	//=====================================================================
	/**
	 * \brief コンストラクタ
	 */
	Player();
	/**
	 * \brief コンストラクタ
	 * \param modelName モデル名
	 * \param objectName オブジェクト名
	 */
	Player(const std::string&		  modelName,
		   std::optional<std::string> objectName = std::nullopt);
	/**
	 * \brief デストラクタ
	 */
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
	 * \brief 移動量の追加をリクエスト
	 * \param delta 移動量
	 */
	void AddMoveRequest(const CalyxMath::Vector3& delta);
	/**
	 * \brief レティクルをオフセット分移動する
	 * \param offset オフセット
	 */
	void MoveReticle(const CalyxMath::Vector3& offset);
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
	void UpdateTilt(const CalyxMath::Vector3& moveVector);
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
	 * \brief 回避をリクエスト
	 */
	void RequestDodge() const;

	/**
	 * \brief 衝突開始
	 * \param other 衝突相手
	 */
	void OnCollisionEnter(Collider* other) override;
	/**
	 * \brief 衝突中
	 * \param other 衝突相手
	 */
	void OnCollisionStay([[maybe_unused]] Collider* other) override {}
	/**
	 * \brief 衝突終了
	 * \param other 衝突相手
	 */
	void OnCollisionExit([[maybe_unused]] Collider* other) override {}

	/* accessor =========================================================== */
	// setter
	/**
	 * \brief 親を設定
	 * \param parent 親のトランスフォーム
	 */
	void SetParent(WorldTransform* parent);
	/**
	 * \brief 敵リストをアタッチ
	 * \param list 敵リスト
	 */
	void AttachEnemyList(const std::list<std::shared_ptr<Enemy>>& list) const;
	/**
	 * \brief 射撃コントローラを設定
	 * \param sc 射撃コントローラ
	 */
	void SetShootingController(std::unique_ptr<PlayerShootingController> sc);

	// getter
	/**
	 * \brief タイプ名を取得
	 * \return タイプ名
	 */
	std::string_view		   GetTypeName() const override { return "Player"; }
	/**
	 * \brief 危険察知を取得
	 * \return 危険察知
	 */
	PlayerDangerSense*		   GetDangerSense() const { return danger_.get(); }
	/**
	 * \brief 回避システムを取得
	 * \return 回避システム
	 */
	PlayerDodgeSystem*		   GetDodgeSystem() const { return dodgeSystem_.get(); }
	/**
	 * \brief 全てのスプライトを取得
	 * \return スプライトリスト
	 */
	std::vector<Sprite*>	   GetAllSprites() const;
	/**
	 * \brief 中心座標を取得
	 * \return 中心座標
	 */
	const CalyxMath::Vector3   GetCenterPos() const override;
	/**
	 * \brief 射撃クールタイムを取得
	 * \return クールタイム
	 */
	std::optional<float>	   GetShootCooldown() const;
	/**
	 * \brief 最大射撃間隔を取得
	 * \return 最大射撃間隔
	 */
	std::optional<const float> GetMaxShootInterval() const;
	/**
	 * \brief レティクルのワールド座標を取得
	 * \return ワールド座標
	 */
	CalyxMath::Vector3		   GetReticleWorldPos() const { return reticleTransform_.GetWorldPosition(); }

private:
	//=====================================================================
	// Private Methods
	//=====================================================================
	/**
	 * \brief レティクルのポジションを更新
	 */
	void UpdateReticlePosition();

	/**
	 * \brief シリアライズ可能パラメータの作成
	 */
	void MakeSerializableParam();

	/**
	 * \brief パラメータパスを取得
	 * \return パラメータパス
	 */
	CalyxEngine::ParamPath GetParamPath() const override;

private:
	//=====================================================================
	// Private Variables
	//=====================================================================
	PlayerMoveController					  moveCtrler_;					 //< 移動コントローラ
	std::unique_ptr<PlayerDodgeSpinMotion>	  dodgeMotion_		  = nullptr; //< 回避モーション
	std::unique_ptr<PlayerDodgeSystem>		  dodgeSystem_		  = nullptr; //< 回避システム
	std::unique_ptr<PlayerDangerSense>		  danger_			  = nullptr; //< 危機察知
	std::unique_ptr<PlayerDamageHandler>	  damageHandler_	  = nullptr; //< ダメージハンドラ
	std::unique_ptr<PlayerLockOn>			  lockOn_			  = nullptr; //< ロックオンシステム
	std::unique_ptr<PlayerShootingController> shootingController_ = nullptr; //< 射撃コントローラ
	PlayerInput								  input_;

	CalyxMath::Vector3 lastMoveVector_;	  //< 最後の移動ベクトル
	WorldTransform	   reticleTransform_; //< レティクルのワールド変換

	// sprites
	std::array<std::unique_ptr<Sprite>, 4> reticleSprites_; //< レティクルのスプライト
	std::unique_ptr<HpGauge>			   hpGauge_;		//< HPゲージ

	// --- Auto Lock-On params ---
	bool autoLockOn_ = true; // オートロックオン有効/無効

	// 画面内クランプ用設定
	bool  clampPlayerInView_  = true;
	bool  clampReticleInView_ = true;
	float clampMarginXpx_	  = 24.0f; // 左右の余白(px)
	float clampMarginYpx_	  = 24.0f; // 上下の余白(px)

	// effect
	std::shared_ptr<CalyxEffect::FxObject> shootFx_;
};
