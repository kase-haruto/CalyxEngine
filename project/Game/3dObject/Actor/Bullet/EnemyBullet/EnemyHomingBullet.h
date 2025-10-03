#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Game/3dObject/Actor/Bullet/BaseBullet.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <string>
#include <algorithm>

class EnemyHomingBullet 
	: public BaseBullet {
public:
	EnemyHomingBullet() = default;
	EnemyHomingBullet(const std::string& modelName, const std::string& name);
	~EnemyHomingBullet() override;

	void ShootInitialize(const Vector3& initPos, const Vector3& velocity) override;
	void Initialize() override;
	void OnShot();
	void SetTarget(const Actor* target);
	void Update(float dt) override;

	void OnCollisionEnter(Collider* other) override;
	void OnCollisionStay([[maybe_unused]] Collider* other) override {}
	void OnCollisionExit([[maybe_unused]] Collider* other) override {}

	const Vector3 GetCenterPos() const override;

	// 調整用
	void SetHomingSpeed(float v) { homingSpeed_ = v; }
	void SetRotateSpeedDegPerSec(float degPerSec) { rotateSpeed_ = degPerSec; }
	void SetGuidance(float g) { guidance_ = std::clamp(g, 0.0f, 1.0f); }
	void SetTrackingNoise(float m);
	// 追尾する秒数（デフォルト1秒）
	void SetHomingDuration(float s);

private:
	const Actor* target_ = nullptr;

	float homingSpeed_ = 2.0f;          // 弾速（ワールド単位/秒）
	float rotateSpeed_ = 120.0f;        // 旋回速度（度/秒）
	float guidance_ = 0.6f;             // 誘導強度 0..1
	float trackingNoiseMeters_ = 0.8f;  // 狙点ノイズ振幅
	float time_ = 0.0f;                 // ノイズ用時間

	// 最初の homingDurationSec_ 秒だけ追尾
	float homingDurationSec_ = 1.0f;
	float homingElapsedSec_ = 0.0f;

	Vector3 baseScale_{ 1.0f, 1.0f, 1.0f };
};