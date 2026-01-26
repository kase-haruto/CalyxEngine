#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// game
#include "Engine/Application/Effects/FxObject.h"
#include "Game/2d/HpGauge.h"
#include "State/Machine/BossStateMachine.h"
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Game/3dObject/Actor/Enemy/EnemyFactionActor.h>
#include <Game/Battle/Shooting/ShootingController/BossShootingController.h>

class BossAI;
class BossAnimController;

/*-----------------------------------------------------------------------------------------
 * Boss
 * - ボスクラス
 * - レールの最後に出現する強敵。倒すことでゲームクリアとなる
 *---------------------------------------------------------------------------------------*/
class Boss final
	: public EnemyFactionActor {
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	/**
	 * \brief コンストラクタ
	 */
	Boss() = default;

	/**
	 * \brief コンストラクタ
	 * \param modelName モデル名
	 * \param objName オブジェクト名
	 */
	Boss(const std::string& modelName, const std::string objName);

	/**
	 * \brief デストラクタ
	 */
	~Boss() override;

	/**
	 * \brief 初期化
	 */
	void Initialize() override;

	/**
	 * \brief AIの初期化
	 */
	void InitializeAI();

	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt) override;

	/**
	 * \brief 派生クラス用GUI表示
	 */
	void DerivativeGui() override;

	/**
	 * \brief ヘッダーGUI表示
	 */
	void HeaderGui() override;

	//--------- collider -----------------------------------------------------
	/**
	 * \brief 衝突開始時処理
	 * \param other 衝突相手のコライダー
	 */
	void OnCollisionEnter(Collider* other) override;

	/**
	 * \brief 衝突継続時処理
	 * \param other 衝突相手のコライダー
	 */
	void OnCollisionStay([[maybe_unused]] Collider* other) override {}

	/**
	 * \brief 衝突終了時処理
	 * \param other 衝突相手のコライダー
	 */
	void OnCollisionExit([[maybe_unused]] Collider* other) override;

	//--------- accessor -----------------------------------------------------
	/**
	 * \brief 中心座標を取得
	 * \return 中心座標
	 */
	const CalyxMath::Vector3 GetCenterPos() const override;
	/**
	 * \brief ターゲットのワールド座標を取得
	 * \return ワールド座標
	 */
	CalyxMath::Vector3 GetTargetWorldPos() const;
	/**
	 * \brief アニメーションコントローラーを取得
	 * \return アニメーションコントローラー
	 */
	BossAnimController* GetAnimator() const;
	/**
	 * \brief AIを取得
	 * \return AI
	 */
	BossAI* GetAI() const;
	/**
	 * \brief 射撃コントローラーを取得
	 * \return 射撃コントローラー
	 */
	BossShootingController* GetShootController() const { return shootingController_.get(); }
	/**
	 * \brief 射撃コントローラーをセット
	 * \param controller 射撃コントローラー
	 */
	void SetShootingController(std::unique_ptr<BossShootingController> controller);
	/**
	 * \brief プレイヤーのトランスフォームをセット
	 * \param target ターゲットアクター
	 */
	void SetPlayerTransform(const Actor* target);
	/**
	 * \brief 全てのスプライト（HPゲージ等）を取得
	 * \return スプライトリスト
	 */
	std::vector<Sprite*> GetAllSprites() const;
	/**
	 * \brief ターゲットアクターを取得
	 * \return ターゲット
	 */
	const Actor* GetTargetActor() const;
	/**
	 * \brief 弾コンテナを取得
	 * \return 弾コンテナ
	 */
	BulletContainer* GetBulletContainer() const { return shootingController_->GetBulletContainer(); }

	bool	IsDebugLoopEnabled() const { return debug.isDebugLoopEnabled; }
	int16_t GetForcedAttackType() const { return debug.forcedAttackType; }

private:
	//===================================================================*/
	//						private methods
	//===================================================================*/
	/**
	 * \brief シリアライズ可能パラメータの初期化
	 */
	void InitializeSerializableParm();

private:
	//===================================================================*/
	//			Inner Class
	//===================================================================*/
	struct BossParam : public CalyxEngine::SerializableObject {
		BossParam();
		CalyxEngine::ParamPath GetParamPath() const override;

		int				   life;
		int				   flinchMax;
		CalyxMath::Vector3 scale;
		CalyxMath::Vector3 initPos;

		struct HpGauge {
			CalyxMath::Vector2 pos; // kGameSize.x * 0.5f = 640
			CalyxMath::Vector2 size;
		} hp;

	} param_;

	struct BossDebugParam {
		bool	isDebugLoopEnabled = false;
		int16_t forcedAttackType   = 0; // BossAttackType
	} debug;
	/**
	 * \brief 死亡時処理
	 */
	void Die();

	/**
	 * \brief プレイヤーの方向を向く
	 */
	void LookAtPlayer();

private:
	//===================================================================*/
	//						private member variables
	//===================================================================*/
	int32_t flinchValue_ = 0; //< ひるみ値
	int32_t flinchMax_	 = 0; //< 最大ひるみ値

	const Actor*							target_				= nullptr; //< プレイヤーのActor
	std::unique_ptr<BossShootingController> shootingController_ = nullptr; //< 発射制御クラス
	std::unique_ptr<BossAI>					ai_					= nullptr; //< AIクラス
	std::unique_ptr<BossStateMachine>		stateMachine_		= nullptr; //< ステートマシン
	std::unique_ptr<BossAnimController>		anim_				= nullptr; //< アニメーションコントローラ
	std::unique_ptr<HpGauge>				hpGauge_			= nullptr; //< HPゲージ
	std::weak_ptr<CalyxEffect::FxObject>	hitEffects_;				   //< ヒットエフェクト群
};