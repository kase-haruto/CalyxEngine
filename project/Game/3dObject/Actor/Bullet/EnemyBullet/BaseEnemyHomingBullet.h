#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include "Engine/Foundation/Utility/Func/CxUtils.h"

#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Game/3dObject/Actor/Bullet/BaseBullet.h>

#include <algorithm>
#include <string>
#include <functional>

/*-----------------------------------------------------------------------------------------
 * EnemyHomingBulletParam
 * - 敵ホーミング弾パラメータ構造体
 *---------------------------------------------------------------------------------------*/
struct EnemyHomingBulletParam : public CalyxEngine::SerializableObject {
	EnemyHomingBulletParam();
	virtual CalyxEngine::ParamPath GetParamPath() const override;

	float homingSpeed		  = 2.0f;
	float rotateSpeed		  = 120.0f;
	float guidance			  = 0.6f;
	float homingDurationSec	  = 1.0f;

	const float scaleFreq = 6.0f;
	const float scaleAmp	 = 0.3f;

protected:
	const std::string& GetSubRootPath() const{
		return subRoot;
	}
	
private:
	std::string subRoot = "Actor/Bullet";
};

/*-----------------------------------------------------------------------------------------
 * BaseEnemyHomingBullet
 * - ターゲット管理、エフェクト管理、追尾計算ロジックを提供する
 * - 敵の追尾弾用 基底クラス
 *---------------------------------------------------------------------------------------*/
class BaseEnemyHomingBullet : public BaseBullet {
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	BaseEnemyHomingBullet() = default;
	BaseEnemyHomingBullet(const std::string& modelName, const std::string& name);
	virtual ~BaseEnemyHomingBullet() override = default;

	/**
	 * \brief 射撃初期化
	 * \param initPos 初期座標
	 * \param velocity 初速
	 */
	void ShootInitialize(const CalyxMath::Vector3& initPos, const CalyxMath::Vector3& velocity) override;
	/**
	 * \brief 初期化
	 */
	void Initialize() override;
	/**
	 * \brief 発射時処理
	 */
	void OnShot() override;
	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt) override;

	//--------- accessor ---------------------------------------------------
	void				 SetTarget(const WorldTransform* target);
	const CalyxMath::Vector3 GetCenterPos() const override;

	// 調整用
	void SetHomingSpeed(float v) { if(param_) param_->homingSpeed = v; }
	void SetRotateSpeedDegPerSec(float degPerSec) { if(param_) param_->rotateSpeed = degPerSec; }
	void SetGuidance(float g) { if(param_) param_->guidance = std::clamp(g, 0.0f, 1.0f); }
	// 追尾する秒数（デフォルト1秒）
	void SetHomingDuration(float s) { if(param_) param_->homingDurationSec = s; }
	void SetTargetPosition(const CalyxMath::Vector3& p) {
		targetPosition_ = p;
	}

protected:
	//===================================================================*/
	//						protected methods
	//===================================================================*/
	/**
	 * \brief 追尾ベクトルを計算する
	 * \param currentVel 現在の速度ベクトル
	 * \param targetPos 目標座標
	 * \param dt デルタタイム
	 * \param speed 弾速
	 * \param rotateSpeedDegPerSec 旋回速度(度/秒)
	 * \param guidance 誘導率 (0.0=慣性のみ ~ 1.0=完全誘導)
	 * \return 新しい速度ベクトル
	 */
	CalyxMath::Vector3 CalculateHomingVelocity(
		const CalyxMath::Vector3& currentVel,
		const CalyxMath::Vector3& targetPos,
		float				  dt,
		float				  speed,
		float				  rotateSpeedDegPerSec,
		float				  guidance = 1.0f) const;

	/**
	 * \brief 指定座標へ向くベクトルを計算（角度制限付き）
	 */
	CalyxMath::Vector3 TurnTowards(
		const CalyxMath::Vector3& currentDir,
		const CalyxMath::Vector3& targetDir,
		float				  maxRad) const;

protected:
	//===================================================================*/
	//						protected members
	//===================================================================*/
	const WorldTransform*				 target_ = nullptr; // 追尾対象
	std::weak_ptr<CalyxEffect::FxObject> trailFx_;			// 軌跡エフェクト
	CalyxMath::Vector3					 baseScale_{1.0f, 1.0f, 1.0f};
	float								 time_			   = 0.0f; // ノイズ用時間
	float								 homingElapsedSec_ = 0.0f; // 最初の homingDurationSec 秒だけ追尾

	// 新しいターゲットプロバイダ（
	std::optional<CalyxMath::Vector3> targetPosition_;

	// デフォルトパラメータ（基底が直接所有する既定値）
	EnemyHomingBulletParam			 defaultParam_;
	// 実際に参照するパラメータ。派生クラスが独自のパラメータを持つ場合は
	// 派生側の param オブジェクトのアドレスをセットすることで共有する。
	EnemyHomingBulletParam*			 param_ = &defaultParam_;
};
