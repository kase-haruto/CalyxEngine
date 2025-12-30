#pragma once

// engine
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineData.h>
#include <Engine/objects/Collider/SphereCollider.h>

// game
#include "Movement/EnemyMovementController.h"
#include "Shoot/EnemyShootingAgent.h"
#include <Game/3dObject/Actor/Enemy/EnemyFactionActor.h>
#include <Game/Battle/Shooting/Pattern/ShootPatternDetails.h>

/*-----------------------------------------------------------------------------------------
 * 敵キャラクターの基本クラス
 * - 敵キャラクタの基本的な振る舞いを実装
 *---------------------------------------------------------------------------------------*/
class Enemy
	: public EnemyFactionActor{
public:
	//===================================================================*/
	//			public types
	//===================================================================*/
	enum class DeathState {
		Alive,
		Dying,
		Dead
	};

public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	Enemy();
	Enemy(const std::string& modelName, const std::string& objName);
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
	 * \brief 衝突時処理
	 * \param other
	 */
	void OnCollisionEnter(Collider* other) override;
	/**
	 * \brief 衝突中処理
	 * \param other
	 */
	void OnCollisionStay(Collider*) override {}
	/**
	 * \brief 衝突終了処理
	 * \param other
	 */
	void OnCollisionExit(Collider*) override {}

	//===================================================================*/
	//			accessore
	//===================================================================*/
	void					 SetPosition(const CalyxMath::Vector3& pos) { worldTransform_.translation = pos; }
	EnemyMovementController* GetMovementController() { return &movement_; }
	void					 SetShootingController(std::unique_ptr<EnemyShootingController>);
	void					 SetPlayerTransform(const WorldTransform* pos);
	void					 SetRouteSpline(const SplineData& data);
	DeathState				 GetDeathState() const { return deathState_; }
	const CalyxMath::Vector3 GetCenterPos() const override;
	void					 SetGameplayEngaged(bool v) { shooting_.SetGameplayEngaged(v); }
	bool					 IsGameplayEngaged() const { return shooting_.IsGameplayEngaged(); }
	void					 SetPatternKind(BulletPatternKind k) { shooting_.SetPatternKind(k); }
	BulletPatternKind		 GetPatternKind() const { return shooting_.GetPatternKind(); }
	void					 EnsurePatternBound() { shooting_.EnsurePatternBound(); }

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	/**
	 * \brief シリアライズ可能パラメータの初期化
	 */
	void InitializeSerializableParm();
	
protected:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	/**
	 * \brief 死亡処理
	 */
	virtual void Die();

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	const WorldTransform* playerTransform_ = nullptr;

	BulletPatternKind			   patternKind_		= BulletPatternKind::SweepFan;
	BulletPatternKind			   lastPatternKind_ = BulletPatternKind::Spiral;
	std::unique_ptr<IShootPattern> pattern_;
	DeathState					   deathState_ = DeathState::Alive;

	// death animation
	CalyxMath::Vector3 deathRotateAxis_ = {0, 0, 1};
	float			   deathTimer_		= 0.0f;
	float			   deathLength_		= 1.5f;

	EnemyMovementController				   movement_; //< 動き制御
	EnemyShootingAgent					   shooting_; //< 射撃制御
	std::shared_ptr<CalyxEffect::FxObject> hitFx_;	  //< ヒットエフェクト
};
