#pragma once

// engine
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineData.h>
#include <Engine/objects/Collider/SphereCollider.h>


// game
#include "Movement/EnemyMovementController.h"
#include "Shoot/EnemyShootingAgent.h"
#include <Game/3dObject/Actor/Enemy/EnemyFactionActor.h>
#include <Game/Battle/Shooting/Pattern/ShootPatternDetails.h>

/*-----------------------------------------------------------------------------------------
 * Enemy
 * - 敵キャラクターの基本クラス
 * - 敵キャラクタの基本的な振る舞い（移動、射撃、死亡処理）を実装
 *---------------------------------------------------------------------------------------*/
class Enemy
	: public EnemyFactionActor {
public:
	//===================================================================*/
	//			public types
	//===================================================================*/
	/**
	 * \brief 死亡状態列挙型
	 */
	enum class DeathState {
		Alive, //< 生存
		Dying, //< 死亡演出中
		Dead   //< 死亡確定
	};

public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	/**
	 * \brief デフォルトコンストラクタ
	 */
	Enemy();

	/**
	 * \brief コンストラクタ
	 * \param modelName モデル名
	 * \param objName オブジェクト名
	 */
	Enemy(const std::string& modelName, const std::string& objName);

	/**
	 * \brief デストラクタ
	 */
	virtual ~Enemy() override;

	/**
	 * \brief 初期化
	 */
	void Initialize() override;

	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt) override;

	/**
	 * \brief フォーメーションへの合流開始
	 * \param formation フォーメーションコントローラー
	 * \param formationOffset フォーメーション内オフセット位置
	 * \param entranceStartWorld フォーメーション合流開始ワールド位置
	 */
	void StartEntranceToFormation(
		EnemyFormationController* formation,
		const CalyxMath::Vector3& formationOffset,
		const CalyxMath::Vector3& entranceStartWorld);

	/**
	 * \brief 衝突開始時処理
	 * \param other 衝突相手のコライダー
	 */
	void OnCollisionEnter(Collider* other) override;

	/**
	 * \brief 衝突継続時処理
	 */
	void OnCollisionStay(Collider*) override {}

	/**
	 * \brief 衝突終了時処理
	 */
	void OnCollisionExit(Collider*) override {}

	//===================================================================*/
	//			accessor
	//===================================================================*/
	/**
	 * \brief 座標をセット
	 * \param pos 座標
	 */
	void SetPosition(const CalyxMath::Vector3& pos) { worldTransform_.translation = pos; }

	/**
	 * \brief 移動コントローラーを取得
	 * \return 移動コントローラー
	 */
	EnemyMovementController* GetMovementController() { return &movement_; }

	/**
	 * \brief 射撃コントローラーをセット
	 * \param controller 射撃コントローラー
	 */
	void SetShootingController(std::unique_ptr<EnemyShootingController> controller);

	/**
	 * \brief プレイヤーのトランスフォームをセット
	 * \param pos トランスフォーム
	 */
	void SetPlayerTransform(const WorldTransform* pos);

	/**
	 * \brief ルートスプラインをセット
	 * \param data スプラインデータ
	 */
	void SetRouteSpline(const SplineData& data);

	/**
	 * \brief 死亡状態を取得
	 * \return 死亡状態
	 */
	DeathState GetDeathState() const { return deathState_; }

	/**
	 * \brief 中心座標を取得
	 * \return 中心座標
	 */
	const CalyxMath::Vector3 GetCenterPos() const override;

	/**
	 * \brief 射撃の開始オフセット（スタッガー）をセット
	 * \param stagger 秒数
	 */
	void SetShootStagger(float stagger);

	/**
	 * \brief ゲームプレイ係合状態をセット
	 * \param v 状態
	 */
	void SetGameplayEngaged(bool v) { shooting_.SetGameplayEngaged(v); }

	/**
	 * \brief ゲームプレイ係合状態か
	 * \return 係合中か
	 */
	bool IsGameplayEngaged() const { return shooting_.IsGameplayEngaged(); }

	/**
	 * \brief 弾パターン種別をセット
	 * \param k パターン種別
	 */
	void SetPatternKind(BulletPatternKind k) { shooting_.SetPatternKind(k); }

	/**
	 * \brief 弾パターン種別を取得
	 * \return パターン種別
	 */
	BulletPatternKind GetPatternKind() const { return shooting_.GetPatternKind(); }

	/**
	 * \brief パターンのバインドを保証
	 */
	void EnsurePatternBound() { shooting_.EnsurePatternBound(); }

protected:
	//===================================================================*/
	//			protected methods
	//===================================================================*/
	/**
	 * \brief 死亡処理
	 */
	virtual void Die();

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	/**
	 * \brief シリアライズ可能パラメータの初期化
	 */
	void InitializeSerializableParm();
	void DerivativeGui() override;

private:
	//===================================================================*/
	//			Inner Class
	//===================================================================*/
	struct EnemyParam : public CalyxEngine::SerializableObject {
		EnemyParam();
		CalyxEngine::ParamPath GetParamPath() const override;

		float			   life;
		float			   killScore;
		float			   moveSpeed;
		CalyxMath::Vector3 scale;

		struct Death {
			float			   length;
			CalyxMath::Vector3 rotateAxis;
		} death;

		struct Collider {
			float radius;
		} col;
	} param_;

private:
	//===================================================================*/
	//			private member variables
	//===================================================================*/
	const WorldTransform* playerTransform_ = nullptr; //< プレイヤーのトランスフォーム

	BulletPatternKind			   patternKind_		= BulletPatternKind::SweepFan; //< 現在の弾パターン
	BulletPatternKind			   lastPatternKind_ = BulletPatternKind::Spiral;   //< 直前の弾パターン
	std::unique_ptr<IShootPattern> pattern_;									   //< 射撃パターンの実体
	DeathState					   deathState_ = DeathState::Alive;				   //< 死亡状態

	// 死亡演出用
	CalyxMath::Vector3 deathRotateAxis_ = {0, 0, 1}; //< 死亡時の回転軸
	float			   deathTimer_		= 0.0f;		 //< 死亡演出タイマー
	float			   deathLength_		= 1.5f;		 //< 死亡演出の長さ

	EnemyMovementController				   movement_; //< 動き制御
	EnemyShootingAgent					   shooting_; //< 射撃制御
	std::shared_ptr<CalyxEffect::FxObject> hitFx_;	  //< ヒットエフェクト
};