#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Foundation/Serialization/SerializableObject.h>
// game
#include "Engine/Application/Effects/FxObject.h"
#include "Game/2d/HpGauge.h"
#include "State/Machine/BossStateMachine.h"
#include <Game/3dObject/Actor/Enemy/EnemyFactionActor.h>
#include <Game/Battle/Shooting/ShootingController/BossShootingController.h>

class BossAI;
class BossAnimController;

/*-----------------------------------------------------------------------------------------
 * Boss class
 * - ボスクラス
 * - レールの最後に出現する敵
 * - 倒したらゲームクリア
 *---------------------------------------------------------------------------------------*/
class Boss final
	: public EnemyFactionActor,
	  public CalyxEngine::SerializableObject {
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	Boss(const std::string& modelName, const std::string objName);
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
	 * \brief GUI表示
	 */
	void DerivativeGui() override;
	/**
	 * \brief ヘッダーGUI表示
	 */
	void HeaderGui() override;

	//--------- collider -----------------------------------------------------
	/**
	 * \brief 衝突時処理
	 */
	void OnCollisionEnter(Collider* other) override;
	/**
	 * \brief 衝突中処理
	 * \param other
	 */
	void OnCollisionStay([[maybe_unused]] Collider* other) override {}
	/**
	 * \brief 衝突終了処理
	 * \param other
	 */
	void OnCollisionExit([[maybe_unused]] Collider* other) override;

	//--------- accessor -----------------------------------------------------
	const CalyxMath::Vector3 GetCenterPos() const override;
	CalyxMath::Vector3		 GetTargetWorldPos() const;
	BossAnimController*		 GetAnimator() const;
	BossAI*					 GetAI() const;
	BossShootingController*	 GetShootController() const { return shootingController_.get(); }
	void					 SetShootingController(std::unique_ptr<BossShootingController>);
	void					 SetPlayerTransform(const Actor* target);
	std::vector<Sprite*>	 GetAllSprites() const;
	const Actor*			 GetTargetActor() const;
	BulletContainer*		 GetBulletContainer() const { return shootingController_->GetBulletContainer(); }
	CalyxEngine::ParamPath	 GetParamPath() const override;

private:
	//===================================================================*/
	//						private methods
	//===================================================================*/
	/**
	 * \brief シリアライズ可能パラメータの初期化
	 */
	void InitializeSerializableParm();
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
	//						private methods
	//===================================================================*/
	int32_t flinchValue_ = 0;
	int32_t flinchMax_	 = 0;

	const Actor*							target_				= nullptr; //< プレイヤーのTransform
	std::unique_ptr<BossShootingController> shootingController_ = nullptr; //< 発射制御クラス
	std::unique_ptr<BossAI>					ai_					= nullptr; //< AIクラス
	std::unique_ptr<BossStateMachine>		stateMachine_		= nullptr; //< ステートマシン
	std::unique_ptr<BossAnimController>		anim_				= nullptr; //< アニメーションコントローラ
	std::unique_ptr<HpGauge>				hpGauge_			= nullptr; //< HPゲージ
	std::weak_ptr<CalyxEffect::FxObject>	hitEffects_;				   //< ヒットエフェクト群
};