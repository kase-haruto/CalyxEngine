#pragma once

#include <memory>

// game shooting
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Game/Battle/Shooting/Pattern/ShootPatternDetails.h>
#include <Game/Battle/Shooting/ShootingController/BulletEmitter.h>
#include <Game/Battle/Shooting/ShootingController/EnemyShootingController.h>


class Enemy;
class WorldTransform;

/*-----------------------------------------------------------------------------------------
 * EnemyShootingAgent class
 * - 敵の射撃処理を統括するクラス
 * - 発射パターンとエミッタの管理を担当する
 *---------------------------------------------------------------------------------------*/
class EnemyShootingAgent {
public:
	EnemyShootingAgent();
	~EnemyShootingAgent();

	/**
	 * \brief 初期化
	 * \param owner 所有者 Enemy
	 */
	void Initialize(Enemy* owner);
	/**
	 * \brief 更新
	 * \param dt デルタタイム
	 */
	void Update(float dt);
	/**
	 * \brief パターンのバインドを保証する
	 */
	void EnsurePatternBound();

	// accessors --------------------------------------------------//
	void			  SetController(std::unique_ptr<EnemyShootingController> ctrl);
	void			  SetTarget(const WorldTransform* tf);
	void			  SetPatternKind(BulletPatternKind k) { patternKind_ = k; }
	BulletPatternKind GetPatternKind() const { return patternKind_; }
	void			  SetGameplayEngaged(bool v) { gameplayEngaged_ = v; }
	bool			  IsGameplayEngaged() const { return gameplayEngaged_; }

	/**
	 * \brief 射撃開始のスタッガーをセット
	 * \param stagger 秒数
	 */
	void SetStagger(float stagger) { initialStagger_ = stagger; }

	void ShowGui();

private:
	//===================================================================*/
	//			Inner Class
	//===================================================================*/
	struct ShootingParam : public CalyxEngine::SerializableObject {
		ShootingParam();
		CalyxEngine::ParamPath GetParamPath() const override;

		float shotsPerSec = 1.0f;
	} param_;

private:
	/**
	 * \brief 発射器を構築する
	 */
	void BuildEmitterIfReady();

private:
	Enemy* owner_ = nullptr;

	const WorldTransform* targetTf_ = nullptr;

	std::unique_ptr<EnemyShootingController> controller_; //< 弾管理
	std::unique_ptr<BulletEmitter>			 emitter_;	  //< 発射器
	std::unique_ptr<IShootPattern>			 pattern_;	  //< 発射パターン

	BulletPatternKind patternKind_	   = BulletPatternKind::AimedNWay;
	BulletPatternKind lastPatternKind_ = BulletPatternKind::Spiral;

	float initialStagger_ = 0.0f; //< 射撃開始のスタッガー

	bool gameplayEngaged_ = false;
};
