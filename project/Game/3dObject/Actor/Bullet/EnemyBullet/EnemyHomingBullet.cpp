#include "EnemyHomingBullet.h"

#include "Engine/Application/Effects/FxObject.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Foundation/Utility/Random/Random.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

	inline float Dot(const CalyxMath::Vector3& a, const CalyxMath::Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
	inline CalyxMath::Vector3 Cross(const CalyxMath::Vector3& a, const CalyxMath::Vector3& b) {
		return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
	}
	inline CalyxMath::Vector3 NormalizeSafe(const CalyxMath::Vector3& v, const CalyxMath::Vector3& fallback = { 0,0,1 }) {
		float len = v.Length();
		if (len <= 1e-6f) return fallback;
		return v / len;
	}
	inline float SafeAcos(float x) {
		if (x < -1.0f) x = -1.0f;
		if (x > 1.0f) x = 1.0f;
		return std::acos(x);
	}
	inline void MakeOrthoBasis(const CalyxMath::Vector3& los, CalyxMath::Vector3& side, CalyxMath::Vector3& up) {
		CalyxMath::Vector3 n = NormalizeSafe(los);
		CalyxMath::Vector3 a = (std::fabs(n.y) < 0.99f) ? CalyxMath::Vector3{ 0,1,0 } : CalyxMath::Vector3{ 1,0,0 };
		side = NormalizeSafe(Cross(n, a));
		up = NormalizeSafe(Cross(side, n));
	}
	inline CalyxMath::Vector3 RotateAroundAxis(const CalyxMath::Vector3& v, const CalyxMath::Vector3& axisUnit, float angle) {
		float c = std::cos(angle), s = std::sin(angle);
		CalyxMath::Vector3 k = axisUnit;
		CalyxMath::Vector3 kxv = Cross(k, v);
		float kdotv = Dot(k, v);
		return v * c + kxv * s + k * (kdotv * (1.0f - c));
	}
	inline CalyxMath::Vector3 TurnTowards(const CalyxMath::Vector3& from, const CalyxMath::Vector3& to, float maxRad) {
		CalyxMath::Vector3 f = NormalizeSafe(from);
		CalyxMath::Vector3 t = NormalizeSafe(to, f);
		float cosang = Dot(f, t);
		float ang = SafeAcos(cosang);
		if (ang <= 1e-6f) return t;
		if (maxRad >= ang) return t;
		CalyxMath::Vector3 axis = NormalizeSafe(Cross(f, t), { 0,1,0 });
		return NormalizeSafe(RotateAroundAxis(f, axis, maxRad));
	}
} // namespace

EnemyHomingBullet::EnemyHomingBullet(const std::string& modelName, const std::string& name)
	: BaseBullet::BaseBullet(modelName, name) {
	this->SetDrawEnable(true);

	trailFx_ = SceneAPI::Instantiate<CalyxEffect::FxObject>("TrailFx");
	auto fx = trailFx_.lock();
	fx->LoadFromPath("Effect/EnemyBulletTrail");
}

EnemyHomingBullet::~EnemyHomingBullet() {}

void EnemyHomingBullet::ShootInitialize(const CalyxMath::Vector3& initPos, const CalyxMath::Vector3& velocity) {
	CalyxMath::Vector3 initDir = (velocity.Length() > 0.001f) ? velocity.Normalize() : CalyxMath::Vector3(0, 0, 1);
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
	CalyxMath::Vector3 rgb = Random::GenerateVector3(0.0f,1.0f);
	CalyxMath::Vector3 color{ rgb };
	model_->SetColor(color);

	baseScale_ = worldTransform_.scale;

	moveSpeed_ = 20.0f;



	auto self = shared_from_this();
	auto fx = trailFx_.lock();
	fx->SetParent(self);
	fx->StopAll();

}

void EnemyHomingBullet::OnShot() {

	auto fx = trailFx_.lock();
	fx->PlayAll();
}

void EnemyHomingBullet::SetTarget(const Actor* target) {
	target_ = target;
}

void EnemyHomingBullet::Update(float dt) {
	time_ += dt;
	homingElapsedSec_ += dt;

	// --- 最初の homingDurationSec 秒だけ誘導 ---
	if (homingElapsedSec_ < homingDurationSec_) {
		if (target_ && target_->GetIsAlive()) {
			const CalyxMath::Vector3 selfPos = GetCenterPos();
			CalyxMath::Vector3 tgtPos = target_->GetCenterPos();

			CalyxMath::Vector3 los = tgtPos - selfPos;
			CalyxMath::Vector3 side, up;
			MakeOrthoBasis(los, side, up);

			const float n1 = std::sin(time_ * 1.73f);
			const float n2 = std::cos(time_ * 2.11f + 1.3f);
			const float amp = trackingNoiseMeters_ * (1.0f - guidance_);
			tgtPos = tgtPos + side * (n1 * amp) + up * (n2 * amp);

			CalyxMath::Vector3 desiredDir = NormalizeSafe(tgtPos - selfPos);
			CalyxMath::Vector3 currentDir = NormalizeSafe(velocity_, CalyxMath::Vector3{ 0,0,1 });

			const float maxRad = (rotateSpeed_ * static_cast<float>(std::numbers::pi) / 180.0f) * dt;
			CalyxMath::Vector3 clampedDir = TurnTowards(currentDir, desiredDir, maxRad);
			CalyxMath::Vector3 newDir = NormalizeSafe(currentDir * (1.0f - guidance_) + clampedDir * guidance_, currentDir);

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

void EnemyHomingBullet::OnCollisionEnter(Collider* ) {
	isAlive_ = false;
}

const CalyxMath::Vector3 EnemyHomingBullet::GetCenterPos() const {
	const CalyxMath::Vector3 offset = { 0.0f, 1.0f, 0.0f };
	return CalyxMath::Vector3::Transform(offset, worldTransform_.matrix.world);
}

void EnemyHomingBullet::SetTrackingNoise(float m) {
	trackingNoiseMeters_ = (std::max)(0.0f, m);
}

void EnemyHomingBullet::SetHomingDuration(float s) { homingDurationSec_ = (std::max)(0.0f, s); }