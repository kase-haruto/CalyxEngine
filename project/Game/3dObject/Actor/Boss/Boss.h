#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Objects/3D/Actor/Actor.h>
// game
#include "Game/2d/HpGauge.h"
#include "State/Machine/BossStateMachine.h"
#include <Game/Battle/Shooting/ShootingController/BossShootingController.h>

class BossAI;
class BossAnimController;
/**
 * \brief レールの最後で出てくるボスキャラクター
 */
class Boss final
	: public Actor {
	enum class DeathState {
		Alive,
		Dying,
		Dead
	};

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
	void OnCollisionExit([[maybe_unused]] Collider* other) override {}

	//--------- accessor -----------------------------------------------------
	const Vector3			GetCenterPos() const override;
	Vector3					GetTargetWorldPos() const;
	BossAnimController*		GetAnimator() const;
	BossAI*					GetAI() const;
	BossShootingController* GetShootController() const { return shootingController_.get(); }
	void					SetShootingController(std::unique_ptr<BossShootingController>);
	void					SetPlayerTransform(const WorldTransform* position);
	std::vector<Sprite*>	GetAllSprites() const;

private:
	//===================================================================*/
	//						private methods
	//===================================================================*/
	DeathState deathState_		= DeathState::Alive;
	Vector3	   deathRotateAxis_ = {0, 0, 1}; // 傾く軸
	Vector3	   basePosition_{};				 // サイン波の基準位置
	float	   waveTime_	  = 0.0f;		 // 経過時間
	float	   waveAmplitude_ = 1.0f;		 // 振れ幅
	float	   waveSpeed_	  = 2.0f;		 // サイン波の速さ
	float	   deathTimer_	  = 0.0f;		 // 死亡演出用
	float	   deathLength_	  = 1.5f;		 // 倒れ終わるまでの秒数

	const WorldTransform*					playerTransform_	= nullptr; //< プレイヤーのTransform
	std::unique_ptr<BossShootingController> shootingController_ = nullptr; //< 発射制御クラス
	std::unique_ptr<BossAI>					ai_					= nullptr; //< AIクラス
	std::unique_ptr<BossStateMachine>		stateMachine_		= nullptr; //< ステートマシン
	std::unique_ptr<BossAnimController>		anim_				= nullptr; //< アニメーションコントローラ
	std::unique_ptr<HpGauge>				hpGauge_			= nullptr; //< HPゲージ
};