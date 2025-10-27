#include "Enemy.h"

// engine
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utility/SceneUtility.h>

// game
#include <Game/Battle/Shooting/Details/AimProvider.h>
#include <Game/Battle/Shooting/Details/FireScheduler.h>
#include <Game/Battle/Shooting/Pattern/PatternCircleRing.h>
#include <Game/Battle/Shooting/Pattern/PatternSweepFan.h>
#include <Game/Battle/Shooting/Score/GainScore.h>
#include <Game/Battle/Shooting/ShootingController/EnemyShootingControllerSink.h>

// stl
#include <numbers>

/* ========================================================================
/* include space
/* ===================================================================== */

Enemy::Enemy() = default;
/////////////////////////////////////////////////////////////////////////////////////////
//      コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
Enemy::Enemy(const std::string& modelName, const std::string objName)
	: Actor(modelName, objName) {
	worldTransform_.scale = {2, 2, 2};

	moveSpeed_ = Random::Generate<float>(1.0f, 3.0f);
	velocity_  = Random::GenerateVector3(-1.0f, 1.0f);

	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	collider_->SetType(ColliderType::Type_Enemy);
	collider_->SetTargetType(ColliderType::Type_PlayerAttack);
	collider_->SetOwner(this);
	if(auto* radius = dynamic_cast<SphereCollider*>(collider_.get())) {
		radius->SetRadius(1.5f);
	}
	collider_->SetIsDrawCollider(false);

	life_		   = 1;
	waveAmplitude_ = 2.0f;
	waveSpeed_	   = Random::Generate<float>(1.0f, 3.0f);

	hitFx_ = SceneAPI::Instantiate<ParticleSystemObject>("hitFx");
	hitFx_->LoadConfig("Effect/HitFx");

	explosionFx_ = SceneAPI::Instantiate<ParticleSystemObject>("explosionFx");
	explosionFx_->LoadConfig("Effect/Explosion");

	// --- スプライン追従の既定値 ---
	mover_.SetWorldSpeed(12.0f);								 // 等速（m/s）
	mover_.SetLookMode(SplineFollower::LookMode::TowardsTarget); // 基本はプレイヤーへ向く
	mover_.SetYOffset(0.0f);
	mover_.SetLoop(false); // デフォルトは終端で停止
}

/////////////////////////////////////////////////////////////////////////////////////////
//      デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
Enemy::~Enemy() {}

/////////////////////////////////////////////////////////////////////////////////////////
//      初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::Initialize() {
	auto self = shared_from_this();

	hitFx_->SetParent(self);
	hitFx_->Stop();

	explosionFx_->SetParent(self);
	explosionFx_->Stop();

	// shootingController_ / playerTransform_ のセット順が分からない場合に備えて
	// ここでは emitter_ を作らず、Update() 内で BuildEmitterIfReady() を呼びます。

	// もし経路が既に与えられていたら、ターゲットTFだけここで関連付け
	if(hasRoute_) {
		mover_.SetTargetTransform(playerTransform_);
	}

	EnsurePatternBound();
}

/////////////////////////////////////////////////////////////////////////////////////////
//      変換
/////////////////////////////////////////////////////////////////////////////////////////
static float Deg2Rad(float d) { return d * std::numbers::pi_v<float> / 180.0f; }

void Enemy::StartStayInCamera(float duration) {
	behaviorState_	= EnemyBehaviorState::StayingInView;
	stayInViewTime_ = 0.0f;
	maxStayTime_	= duration;

	// カメラを親に設定して相対位置固定
	if(auto camSp = CameraManager::GetMain3dShared()) {
		worldTransform_.parent = &camSp->GetWorldTransform();
	}
	worldTransform_.translation = Vector3(0.0f, 0.0f, 30.0f);
}

/* ========================================================================
   カメラ前方ステイ処理
   ===================================================================== */
void Enemy::StayInView(float dt) {
	stayInViewTime_ += dt;

	const float yOffset = std::sinf(stayInViewTime_ * 2.0f) * 0.5f;
	worldTransform_.translation = Vector3(0, yOffset, 40.0f);

	// --------- プレイヤーの方向を向く処理 ---------
	if (playerTransform_ && worldTransform_.parent) {
		Vector3 targetWorld = playerTransform_->GetWorldPosition();
		Vector3 myWorld     = GetWorldPosition();

		const Matrix4x4& parentWorld = worldTransform_.parent->matrix.world;
		Matrix4x4 invParent = Matrix4x4::Inverse(parentWorld);

		Vector3 targetLocal = Vector3::Transform(targetWorld, invParent);
		Vector3 myLocal     = Vector3::Transform(myWorld, invParent);

		Vector3 dir = (targetLocal - myLocal).Normalize();
		if (dir.LengthSquared() > 1e-12f) {
			const float yaw = std::atan2(dir.x, dir.z);
			const float pitch = std::atan2(-dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z));
			worldTransform_.rotation = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
		}
	}

	if (stayInViewTime_ >= maxStayTime_) {
		behaviorState_ = EnemyBehaviorState::Active;
	}
}

////////////////////////////////////////////////////////////////
//  Update
////////////////////////////////////////////////////////////////
void Enemy::Update(float dt) {
	if(deathState_ == DeathState::Alive) {
		// --- ステイ中なら専用処理 ---
		if(behaviorState_ == EnemyBehaviorState::StayingInView) {
			StayInView(dt);
			return;
		}

		if(life_ <= 0) {
			deathState_ = DeathState::Dying;
			explosionFx_->Play();
			deathTimer_		 = 0.0f;
			deathRotateAxis_ = {1, 0, 0};
			return;
		}

		BuildEmitterIfReady();
		
		// if (hasRoute_) {
		// 	mover_.SetTargetTransform(playerTransform_);
		// 	waveTime_ += dt * waveSpeed_;
		// 	const float offsetY = std::sin(waveTime_) * waveAmplitude_;
		// 	worldTransform_.translation = Vector3{0, offsetY, 0};
		//
		// 	const Vector3 myPos = worldTransform_.translation;
		// 	const Vector3 targetPos = playerTransform_ ? playerTransform_->GetWorldPosition() : myPos;
		// 	Vector3 d = (targetPos - myPos).Normalize();
		// 	if (d.LengthSquared() > 1e-12f) {
		// 		const float yaw = std::atan2(d.x, d.z);
		// 		const float pitch = std::atan2(-d.y, std::sqrt(d.x * d.x + d.z * d.z));
		// 		worldTransform_.rotation = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
		// 	}
		// }
		// else {
		// 	waveTime_ += dt * waveSpeed_;
		// 	const float offsetY = std::sin(waveTime_) * waveAmplitude_;
		// 	worldTransform_.translation = basePosition_ + Vector3{0, offsetY, 0};
		// }

		// 方向合わせ（プレイヤーへ）
		if (playerTransform_) {
			Vector3 targetWorld = playerTransform_->GetWorldPosition();
			Vector3 myWorld = GetWorldPosition();

			// カメラのワールド行列（親）
			const Matrix4x4& camWorld = worldTransform_.parent->matrix.world;

			// カメラのワールド逆行列
			Matrix4x4 invParent = Matrix4x4::Inverse(camWorld);

			// ワールド空間のターゲット位置をカメラのローカル空間へ
			Vector3 targetLocal = Vector3::Transform(targetWorld, invParent);
			Vector3 myLocal = Vector3::Transform(myWorld, invParent);

			Vector3 dir = (targetLocal - myLocal).Normalize();
			if (dir.LengthSquared() > 1e-12f) {
				const float yaw = std::atan2(dir.x, dir.z);
				const float pitch = std::atan2(-dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z));
				worldTransform_.rotation = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
			}
		}

		if(shootingController_) {
			shootingController_->SetGameplayEngaged(this->IsGameplayEngaged());
			shootingController_->Update(dt);
		}

		if(this->IsGameplayEngaged() && emitter_) {
			if(auto* pat = emitter_->Pattern()) {
				pat->Advance(dt);
			}

			BulletEmitterContext cxt{};
			cxt.origin	  = GetCenterPos();
			cxt.targetPos = playerTransform_ ? playerTransform_->GetWorldPosition() : GetWorldPosition();

			emitter_->Update(dt, cxt);
		}
		return;
	}

	if(deathState_ == DeathState::Dying) {
		deathTimer_ += dt;
		float t = std::clamp(deathTimer_ / deathLength_, 0.0f, 1.0f);

		float rad				 = std::numbers::pi_v<float> * 0.5f * t;
		worldTransform_.rotation = Quaternion::MakeRotateAxisQuaternion(deathRotateAxis_, rad);

		if(t >= 1.0f && !explosionFx_->IsPlaying()) {
			deathState_ = DeathState::Dead;
			deathTimer_ = 0.0f;
		}
		return;
	}

	if(deathState_ == DeathState::Dead) {
		this->Die();
		isAlive_ = false;
		return;
	}
}

void Enemy::EnsurePatternBound() {
	if(!emitter_) return;

	if(!pattern_ || patternKind_ != lastPatternKind_) {
		pattern_		 = CreatePattern(patternKind_);
		lastPatternKind_ = patternKind_;
	}
	emitter_->SetPattern(pattern_.get()); // 非所有参照を差し替え
}

////////////////////////////////////////////////////////////////
//  衝突
////////////////////////////////////////////////////////////////
void Enemy::OnCollisionEnter(Collider*) {
	if(life_ >= 1) {
		life_--;
		hitFx_->Play();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//      中心座標取得
/////////////////////////////////////////////////////////////////////////////////////////
const Vector3 Enemy::GetCenterPos() const {
	const Vector3 offset   = {0.0f, 1.5f, 0.0f};
	Vector3		  worldPos = Vector3::Transform(offset, worldTransform_.matrix.world);
	return worldPos;
}

// スコア取得
int16_t Enemy::GetScore() const {
	{
		return score_;
	}
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

/////////////////////////////////////////////////////////////////////////////////////////
//      移動（未使用：スプライン追従が担当）
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::Move() {}

/////////////////////////////////////////////////////////////////////////////////////////
//      弾発射
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::Shoot() {
	Vector3 myPos = GetCenterPos();
	Vector3 dir	  = Vector3(playerTransform_->GetWorldPosition() - myPos).Normalize();
	shootingController_->RequestShoot(GetCenterPos(), dir);
}

/////////////////////////////////////////////////////////////////////////////////////////
//      死亡時処理
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::Die() {
	GainScore event;
	event.amount = score_;
	event.reason = "enemyKill";
	event.tag	 = {"normal"};
	EventBus::Publish(event);
}

/////////////////////////////////////////////////////////////////////////////////////////
//      エミッタの一度だけ生成する関数
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::BuildEmitterIfReady() {
	if(emitter_) return;
	if(!shootingController_) return;
	if(!playerTransform_) return;

	auto sink = std::make_unique<EnemyShootingControllerSink>(shootingController_.get());
	auto aim  = std::make_unique<AimAtTarget>();

	FireScheduler sched;
	sched.shotsPerSec = 6.0f;

	BulletEmitterConfig cfg;
	cfg.tag = "enemy_homing";

	// ここでは“パターンなし”で作る（SetPatternで後から差し込む）
	emitter_ = std::make_unique<BulletEmitter>(
		cfg, std::move(sink), std::move(aim), nullptr, sched);

	EnsurePatternBound();
}