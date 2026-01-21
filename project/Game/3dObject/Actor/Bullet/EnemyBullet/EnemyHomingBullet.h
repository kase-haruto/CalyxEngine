#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Game/3dObject/Actor/Bullet/BaseBullet.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <string>
#include <algorithm>

namespace CalyxEffect {
	class FxObject;
}

/*-----------------------------------------------------------------------------------------
 * EnemyHomingBullet
 * - 敵ホーミング弾クラス
 * - プレイヤーを追尾するホーミング弾の挙動を実装
 *---------------------------------------------------------------------------------------*/
class EnemyHomingBullet
	: public BaseBullet {
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	EnemyHomingBullet() = default;
	EnemyHomingBullet(const std::string& modelName, const std::string& name);
	~EnemyHomingBullet() override;

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
	void OnShot();
	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void SetTarget(const WorldTransform* target);
	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt) override;


	// -------- collider -------------------------------------------------
	void OnCollisionEnter(Collider* other) override;
	void OnCollisionStay([[maybe_unused]] Collider* other) override {}
	void OnCollisionExit([[maybe_unused]] Collider* other) override {}

	const CalyxMath::Vector3 GetCenterPos() const override;

	// 調整用
	void SetHomingSpeed(float v) { param_.homingSpeed = v; }
	void SetRotateSpeedDegPerSec(float degPerSec) { param_.rotateSpeed = degPerSec; }
	void SetGuidance(float g) { param_.guidance = std::clamp(g, 0.0f, 1.0f); }
	void SetTrackingNoise(float m);
	// 追尾する秒数（デフォルト1秒）
	void SetHomingDuration(float s);

private:
	const WorldTransform* target_ = nullptr;

	struct EnemyHomingBulletParam 
	: public CalyxEngine::SerializableObject	{
		EnemyHomingBulletParam();
		CalyxEngine::ParamPath GetParamPath() const override;

		float homingSpeed = 2.0f;
		float rotateSpeed = 120.0f;
		float guidance = 0.6f;
		float trackingNoiseMeters = 0.8f;
		float homingDurationSec = 1.0f;

		const float scaleFreq = 6.0f;
		const float scaleAmp = 0.3f;

	}param_;

	float time_ = 0.0f;                 // ノイズ用時間

	// 最初の homingDurationSec_ 秒だけ追尾
	float homingElapsedSec_ = 0.0f;

	std::weak_ptr<CalyxEffect::FxObject> trailFx_; //< 発射エフェクト

	CalyxMath::Vector3 baseScale_{ 1.0f, 1.0f, 1.0f };
};