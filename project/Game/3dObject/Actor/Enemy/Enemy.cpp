#include "Enemy.h"

// engine
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Objects/Collider/SphereCollider.h>
#include <Engine/Scene/Utility/SceneUtility.h>

// game
#include <Game/Battle/Shooting/Score/GainScore.h>

#include <numbers>

Enemy::Enemy() = default;

Enemy::Enemy(const std::string& modelName, const std::string objName)
	: Actor(modelName, objName) {

	worldTransform_.scale = {2, 2, 2};

	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	if(auto* c = dynamic_cast<SphereCollider*>(collider_.get())) {
		c->SetRadius(1.5f);
	}
	collider_->SetType(ColliderType::Type_Enemy);
	collider_->SetTargetType(ColliderType::Type_PlayerAttack);
	collider_->SetOwner(this);

	// hit effect
	hitFx_ = SceneAPI::Instantiate<FxObject>("HitFx");
	hitFx_->LoadFromPath("Effect/HitEffect");
}

Enemy::~Enemy() = default;

void Enemy::Initialize() {
	auto self = shared_from_this();
	hitFx_->SetParent(self);
	hitFx_->StopAll();

	// movement / shooting 初期化
	movement_.Initialize(this);
	shooting_.Initialize(this);
}

void Enemy::StartStayInCamera(float duration) {
	behaviorState_ = EnemyBehaviorState::StayingInView;
	stayInViewTime_ = 0.0f;
	maxStayTime_ = duration;

	if(auto camSp = CameraManager::GetMain3dShared()) {
		worldTransform_.parent = &camSp->GetWorldTransform();
	}

	// Z だけは一定、X/Y はランダムに散らす
	const float baseZ = 80.0f;

	float randX = Random::Generate<float>(-30.0f, 30.0f);
	float randY = Random::Generate<float>(-20.0f, 20.0f);

	camAnchor_ = Vector3(randX, randY, baseZ);

	// ランダム位相
	camPhaseX_ = Random::Generate<float>(0.0f, 6.28318f);
	camPhaseY_ = Random::Generate<float>(0.0f, 6.28318f);
	camPhaseZ_ = Random::Generate<float>(0.0f, 6.28318f);
}
/* ========================================================================
   カメラ前方ステイ処理
   ===================================================================== */
void Enemy::StayInView(float dt) {
	// 時間だけ加算
	stayInViewTime_ += dt;

	// ここでドリフト（ローカルXYを揺らし＆画面内クランプ＆向き合わせ）
	UpdateCameraSpaceDrift(dt);

	// 規定時間を超えたら退場へ
	if(stayInViewTime_ >= maxStayTime_) {
		BeginExitFromCamera();
	}
	movement_.StartStay(duration);
}

void Enemy::SetPlayerTransform(const WorldTransform* tf) {
	playerTransform_ = tf;
	movement_.SetPlayerTransform(tf);
	shooting_.SetTarget(tf);
}

void Enemy::SetRouteSpline(const SplineData& data) {
	movement_.SetRoute(data, playerTransform_);
}

void Enemy::SetShootingController(std::unique_ptr<EnemyShootingController> controller) {
	shooting_.SetController(std::move(controller));
}

void Enemy::Update(float dt) {

	// death check
	if(deathState_ == DeathState::Alive) {
		if(life_ <= 0) {
			deathState_		 = DeathState::Dying;
			deathTimer_		 = 0.0f;
			deathRotateAxis_ = {1, 0, 0};
			return;
		}
	}

	// movement
	movement_.Update(dt);

	// shooting
	shooting_.Update(dt);

	// death anim
	if(deathState_ == DeathState::Dying) {

		deathTimer_ += dt;
		float t = std::clamp(deathTimer_ / deathLength_, 0.0f, 1.0f);

		float rad = std::numbers::pi_v<float> * 0.5f * t;
		worldTransform_.rotation =
			Quaternion::MakeRotateAxisQuaternion(deathRotateAxis_, rad);

		// 色を薄くしていく
		Vector4 color = GetModel()->GetColor();
		color.w		 = 1.0f - t;
		GetModel()->SetColor(color);

		// タイマー経過でも Dead へ進める
		if(t >= 1.0f ) {
			deathState_ = DeathState::Dead;
			deathTimer_ = 0.0f;
		}
		return;
	}

	if(deathState_ == DeathState::Dead) {
		Die();
		isAlive_ = false;
		return;
	}
}

void Enemy::OnCollisionEnter(Collider*) {
	if(life_ >= 1) {
		life_--;
		hitFx_->PlayAll();
	}
}

const Vector3 Enemy::GetCenterPos() const {
	const Vector3 offset   = {0.0f, 1.5f, 0.0f};
	Vector3		  worldPos = Vector3::Transform(offset, worldTransform_.matrix.world);
	return worldPos;
}

// スコア取得
int16_t Enemy::GetScore() const {
		return score_;
}

/////////////////////////////////////////////////////////////////////////////////////////
//      弾コントロール
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::SetShootingController(std::unique_ptr<EnemyShootingController> controller) { shootingController_ = std::move(controller); }

/////////////////////////////////////////////////////////////////////////////////////////
//      プレイヤーのtfをセット
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::SetPlayerTransform(const WorldTransform* tf) {
	playerTransform_ = tf;
	// 追従の向きが TowardsTarget のときに参照される
	mover_.SetTargetTransform(playerTransform_);
}

/////////////////////////////////////////////////////////////////////////////////////////
//      移動ルートせってい
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::SetRouteSpline(const SplineData& data) {
	moveRoute_ = data;
	moveRoute_.BuildArcTable(); // 等速化のLUTを構築
	mover_.BindPath(&moveRoute_);
	mover_.SetLookMode(SplineFollower::LookMode::TowardsTarget);
	mover_.SetTargetTransform(playerTransform_);
	hasRoute_ = (moveRoute_.SegmentCount() > 0);
}

int16_t Enemy::GetScore() const { return score_; }

void Enemy::Die() {
	GainScore e;
	e.amount = score_;
	e.reason = "enemyKill";
	e.tag	 = {"normal"};
	EventBus::Publish(e);
}

void Enemy::UpdateCameraSpaceDrift(float /*dt*/) {
	auto cam = CameraManager::GetMain3d();
	if(!cam || !worldTransform_.parent) return; // ← 親チェックを追加

	const float t = ClockManager::GetInstance()->GetTotalTime();

	// ふらつき初期値ガード（すべて 0 のときにデフォルトを入れる）
	if(std::abs(camDriftAmpX_) < 1e-6f && std::abs(camDriftAmpY_) < 1e-6f && std::abs(camDriftAmpZ_) < 1e-6f) {
		camDriftAmpX_ = 8.0f;
		camDriftAmpY_ = 4.5f;
		camDriftAmpZ_ = 2.0f;
	}
	if(camPhaseX_ == 0 && camPhaseY_ == 0 && camPhaseZ_ == 0) {
		camPhaseX_ = Random::Generate<float>(0.0f, 6.28318f);
		camPhaseY_ = Random::Generate<float>(0.0f, 6.28318f);
		camPhaseZ_ = Random::Generate<float>(0.0f, 6.28318f);
	}
	if(camAnchor_.z <= 0.0f)
		camAnchor_.z = 55.0f; // XY はそのまま

	// Lissajous 風ドリフト
	const float dx = std::sin((t + camPhaseX_) * camDriftFreqX_) * camDriftAmpX_;
	const float dy = std::sin((t + camPhaseY_) * camDriftFreqY_) * camDriftAmpY_;
	const float dz = std::sin((t + camPhaseZ_) * camDriftFreqZ_) * camDriftAmpZ_;
	Vector3		p  = camAnchor_ + Vector3{dx, dy, dz};

	// 画面内クランプ
	const float fovY   = cam->GetFovY();
	const float aspect = cam->GetAspectRatio(); // 命名を統一
	ClampToCameraFrustumXY(p, fovY, aspect, camDriftMargin_);
	worldTransform_.translation = p;

	// 親=カメラ基準でプレイヤーの方向へ向く
	if(playerTransform_) {
		const Matrix4x4& parentWorld = worldTransform_.parent->matrix.world;
		Matrix4x4		 invParent	 = Matrix4x4::Inverse(parentWorld);
		Vector3			 targetLocal = Vector3::Transform(playerTransform_->GetWorldPosition(), invParent);
		Vector3			 myLocal	 = Vector3::Transform(GetWorldPosition(), invParent);
		Vector3			 dir		 = (targetLocal - myLocal).Normalize();
		if(dir.LengthSquared() > 1e-12f) {
			const float yaw			 = std::atan2(dir.x, dir.z);
			const float pitch		 = std::atan2(-dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z));
			worldTransform_.rotation = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
		}
	}
}
