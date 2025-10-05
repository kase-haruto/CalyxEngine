#include "EnemyHomingBullet.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Foundation/Utility/Random/Random.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

	inline float Dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
	inline Vector3 Cross(const Vector3& a, const Vector3& b) {
		return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
	}
	inline Vector3 NormalizeSafe(const Vector3& v, const Vector3& fallback = { 0,0,1 }) {
		float len = v.Length();
		if (len <= 1e-6f) return fallback;
		return v / len;
	}
	inline float SafeAcos(float x) {
		if (x < -1.0f) x = -1.0f;
		if (x > 1.0f) x = 1.0f;
		return std::acos(x);
	}
	inline void MakeOrthoBasis(const Vector3& los, Vector3& side, Vector3& up) {
		Vector3 n = NormalizeSafe(los);
		Vector3 a = (std::fabs(n.y) < 0.99f) ? Vector3{ 0,1,0 } : Vector3{ 1,0,0 };
		side = NormalizeSafe(Cross(n, a));
		up = NormalizeSafe(Cross(side, n));
	}
	inline Vector3 RotateAroundAxis(const Vector3& v, const Vector3& axisUnit, float angle) {
		float c = std::cos(angle), s = std::sin(angle);
		Vector3 k = axisUnit;
		Vector3 kxv = Cross(k, v);
		float kdotv = Dot(k, v);
		return v * c + kxv * s + k * (kdotv * (1.0f - c));
	}
	inline Vector3 TurnTowards(const Vector3& from, const Vector3& to, float maxRad) {
		Vector3 f = NormalizeSafe(from);
		Vector3 t = NormalizeSafe(to, f);
		float cosang = Dot(f, t);
		float ang = SafeAcos(cosang);
		if (ang <= 1e-6f) return t;
		if (maxRad >= ang) return t;
		Vector3 axis = NormalizeSafe(Cross(f, t), { 0,1,0 });
		return NormalizeSafe(RotateAroundAxis(f, axis, maxRad));
	}
} // namespace

EnemyHomingBullet::EnemyHomingBullet(const std::string& modelName, const std::string& name)
	: BaseBullet::BaseBullet(modelName, name) {
	this->SetDrawEnable(true);
}

EnemyHomingBullet::~EnemyHomingBullet() {}

void EnemyHomingBullet::ShootInitialize(const Vector3& initPos, const Vector3& velocity) {
	Vector3 initDir = (velocity.Length() > 0.001f) ? velocity.Normalize() : Vector3(0, 0, 1);
	BaseBullet::ShootInitialize(initPos, initDir * homingSpeed_);
	time_ = 0.0f;
	homingElapsedSec_ = 0.0f;
}

void EnemyHomingBullet::Initialize() {

	collider_->SetType(ColliderType::Type_EnemyAttack);
	collider_->SetTargetType(ColliderType::Type_Player);
	collider_->SetOwner(this);
	if (auto* box = dynamic_cast<BoxCollider*>(collider_.get())) {
		box->SetSize({ 1.5f, 1.5f, 1.5f });
	}
	BaseGameObject::SetTexture("white1x1.png");
	BaseGameObject::SetBillboardMode(BillboardMode::Full);

	//ライティングなし
	BaseGameObject::SetLightingMode(LightingMode::UnlitColor);
	BaseGameObject::SetBlendMode(BlendMode::ADD);
	Vector3 rgb = Random::GenerateVector3(0.0f,1.0f);
	Vector3 color{ rgb };
	model_->SetColor(color);

	baseScale_ = worldTransform_.scale;

	moveSpeed_ = 25.0f;
}

void EnemyHomingBullet::OnShot() {}

void EnemyHomingBullet::SetTarget(const Actor* target) {
	target_ = target;
}

void EnemyHomingBullet::Update(float dt) {
	time_ += dt;
	homingElapsedSec_ += dt;

	// --- 最初の homingDurationSec 秒だけ誘導 ---
	if (homingElapsedSec_ < homingDurationSec_) {
		if (target_ && target_->GetIsAlive()) {
			const Vector3 selfPos = GetCenterPos();
			Vector3 tgtPos = target_->GetCenterPos();

			Vector3 los = tgtPos - selfPos;
			Vector3 side, up;
			MakeOrthoBasis(los, side, up);

			const float n1 = std::sin(time_ * 1.73f);
			const float n2 = std::cos(time_ * 2.11f + 1.3f);
			const float amp = trackingNoiseMeters_ * (1.0f - guidance_);
			tgtPos = tgtPos + side * (n1 * amp) + up * (n2 * amp);

			Vector3 desiredDir = NormalizeSafe(tgtPos - selfPos);
			Vector3 currentDir = NormalizeSafe(velocity_, Vector3{ 0,0,1 });

			const float maxRad = (rotateSpeed_ * static_cast<float>(std::numbers::pi) / 180.0f) * dt;
			Vector3 clampedDir = TurnTowards(currentDir, desiredDir, maxRad);
			Vector3 newDir = NormalizeSafe(currentDir * (1.0f - guidance_) + clampedDir * guidance_, currentDir);

			velocity_ = newDir * homingSpeed_;
		}
	}

	// --- scale をうねうね揺らす ---
	const float freq = 6.0f;
	const float amp = 0.3f;

	float sx = 1.0f + amp * std::sin(time_ * freq);
	float sy = 1.0f + amp * std::cos(time_ * freq);

	// 基準スケールに対して相対的に揺らす
	worldTransform_.scale.x = baseScale_.x * sx;
	worldTransform_.scale.y = baseScale_.y * sy;
	worldTransform_.scale.z = baseScale_.z;

	BaseBullet::Update(dt);
}

void EnemyHomingBullet::OnCollisionEnter(Collider* other) {
	if (!other) return;
	// 衝突マネージャが対象外を弾いている前提なら、タイプチェックは緩めでOK
	if (other->GetType() != ColliderType::Type_Player) return;

	isAlive_ = false;
}

const Vector3 EnemyHomingBullet::GetCenterPos() const {
	const Vector3 offset = { 0.0f, 1.0f, 0.0f };
	return Vector3::Transform(offset, worldTransform_.matrix.world);
}

void EnemyHomingBullet::SetTrackingNoise(float m) {
	trackingNoiseMeters_ = (std::max)(0.0f, m);
}

void EnemyHomingBullet::SetHomingDuration(float s) { homingDurationSec_ = (std::max)(0.0f, s); }