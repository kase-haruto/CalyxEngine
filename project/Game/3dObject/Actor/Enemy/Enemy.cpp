#include "Enemy.h"
/* ========================================================================
/* include space
/* ===================================================================== */
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

namespace {

void ClampToCameraFrustumXY(Vector3& localPos, float fovY, float aspect, float margin) {
	// Z>0 前方。Zに応じて見えていいXYの最大幅を計算
	const float halfH = localPos.z * std::tan(fovY * 0.5f);
	const float halfW = halfH * aspect;
	const float maxX  = halfW * margin;
	const float maxY  = halfH * margin;
	localPos.x		  = std::clamp(localPos.x, -maxX, maxX);
	localPos.y		  = std::clamp(localPos.y, -maxY, maxY);
}

} // namespace

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

	hitFx_ = SceneAPI::Instantiate<FxObject>("HitFx");
	hitFx_->LoadFromPath("Effect/HitEffect");

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
	hitFx_->StopAll();

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

	if(auto camSp = CameraManager::GetMain3dShared()) {
		worldTransform_.parent = &camSp->GetWorldTransform();
	}
	// 画面中央ちょい奥を基準に
	camAnchor_ = Vector3(0.0f, 0.0f, 55.0f);

	// ランダム位相を入れて個体差を出す
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
}

// 画面外へ出るための初期化：向かう先を決める
void Enemy::BeginExitFromCamera() {
	exitPrepared_ = false;
	auto camSp	  = CameraManager::GetMain3dShared();
	if(!camSp || !worldTransform_.parent) return;

	// 現在のローカル位置
	Vector3 p = worldTransform_.translation;

	// 現在の深度におけるローカルXYの可視範囲
	const float fovY   = camSp->GetFovY(); // [rad]
	const float aspect = camSp->GetAspectRatio();
	const float halfH  = p.z * std::tan(fovY * 0.5f);
	const float halfW  = halfH * aspect;

	// ランダムに左右どちらかの画面外へ抜ける
	const float side  = (Random::Generate<float>(0.f, 1.f) < 0.5f) ? -1.f : 1.f;
	const float edgeX = side * halfW * 1.2f; // 画面外ちょい先
	const float edgeY = Random::Generate<float>(-halfH * 0.8f, halfH * 0.8f);

	Vector3 targetLocal{edgeX, edgeY, p.z}; // 深度はそのまま
	Vector3 dir = (targetLocal - p).Normalize();
	if(dir.LengthSquared() < 1e-8f) dir = {side, 0, 0}; // 保険

	exitDirLocal_  = dir;
	exitPrepared_  = true;
	behaviorState_ = EnemyBehaviorState::ExitingView;
}

// 退場のフレーム更新
void Enemy::UpdateExitFromCamera(float dt) {
	auto camSp = CameraManager::GetMain3dShared();
	if(!camSp || !worldTransform_.parent) {
		isAlive_ = false;
		return;
	}

	// 進行
	Vector3 p = worldTransform_.translation;
	p += exitDirLocal_ * (exitSpeedLocal_ * dt);
	worldTransform_.translation = p;

	if(playerTransform_) {
		// 親（カメラ）ローカルでの向き合わせ
		Vector3	  targetWorld = playerTransform_->GetWorldPosition();
		Vector3	  myWorld	  = GetWorldPosition();
		Matrix4x4 invParent	  = Matrix4x4::Inverse(worldTransform_.parent->matrix.world);
		Vector3	  targetLocal = Vector3::Transform(targetWorld, invParent);
		Vector3	  myLocal	  = Vector3::Transform(myWorld, invParent);
		Vector3	  dir		  = (targetLocal - myLocal).Normalize();
		if(dir.LengthSquared() > 1e-12f) {
			const float yaw			 = std::atan2(dir.x, dir.z);
			const float pitch		 = std::atan2(-dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z));
			worldTransform_.rotation = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
		}
	}

	// 画面外判定：現在深度の可視範囲を超えたら消滅
	const float fovY   = camSp->GetFovY();
	const float aspect = camSp->GetAspectRatio();
	const float halfH  = p.z * std::tan(fovY * 0.5f);
	const float halfW  = halfH * aspect;

	const bool outX = std::abs(p.x) > halfW * exitOvershoot_;
	const bool outY = std::abs(p.y) > halfH * exitOvershoot_;
	if(outX || outY) {
		// 無得点でフェードアウト扱い（爆発もスコアも無し）
		isAlive_ = false;
		return;
	}
}

////////////////////////////////////////////////////////////////
//  Update
////////////////////////////////////////////////////////////////
void Enemy::Update(float dt) {
	if(deathState_ == DeathState::Alive) {

		//  どのステイトでも最優先で死亡へ
		if(life_ <= 0) {
			deathState_ = DeathState::Dying;
			deathTimer_		 = 0.0f;
			deathRotateAxis_ = {1, 0, 0};
			return;
		}

		//  ステイ中
		if(behaviorState_ == EnemyBehaviorState::StayingInView) {
			StayInView(dt);

			BuildEmitterIfReady();
			if(shootingController_) {
				shootingController_->SetGameplayEngaged(IsGameplayEngaged());
				shootingController_->Update(dt);
			}
			if(IsGameplayEngaged() && emitter_) {
				if(auto* pat = emitter_->Pattern()) pat->Advance(dt);
				BulletEmitterContext cxt{};
				cxt.origin	  = GetCenterPos();
				cxt.targetPos = playerTransform_ ? playerTransform_->GetWorldPosition() : GetWorldPosition();
				emitter_->Update(dt, cxt);
			}
			return;
		}

		//  退場中
		if(behaviorState_ == EnemyBehaviorState::ExitingView) {
			if(!exitPrepared_) BeginExitFromCamera();
			UpdateExitFromCamera(dt);

			BuildEmitterIfReady();
			if(shootingController_) {
				shootingController_->SetGameplayEngaged(IsGameplayEngaged());
				shootingController_->Update(dt);
			}
			if(IsGameplayEngaged() && emitter_) {
				if(auto* pat = emitter_->Pattern()) pat->Advance(dt);
				BulletEmitterContext cxt{};
				cxt.origin	  = GetCenterPos();
				cxt.targetPos = playerTransform_ ? playerTransform_->GetWorldPosition() : GetWorldPosition();
				emitter_->Update(dt, cxt);
			}
			return;
		}

		//  Active（通常）
		BuildEmitterIfReady();

		// 向き合わせ（親の有無で空間切替）
		if(playerTransform_) {
			Vector3 targetWorld = playerTransform_->GetWorldPosition();
			Vector3 myWorld		= GetWorldPosition();
			Vector3 dir;
			if(worldTransform_.parent) {
				Matrix4x4 invParent	  = Matrix4x4::Inverse(worldTransform_.parent->matrix.world);
				Vector3	  targetLocal = Vector3::Transform(targetWorld, invParent);
				Vector3	  myLocal	  = Vector3::Transform(myWorld, invParent);
				dir					  = (targetLocal - myLocal).Normalize();
			} else {
				dir = (targetWorld - myWorld).Normalize();
			}
			if(dir.LengthSquared() > 1e-12f) {
				const float yaw			 = std::atan2(dir.x, dir.z);
				const float pitch		 = std::atan2(-dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z));
				worldTransform_.rotation = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
			}
		}

		if(shootingController_) {
			shootingController_->SetGameplayEngaged(IsGameplayEngaged());
			shootingController_->Update(dt);
		}
		if(IsGameplayEngaged() && emitter_) {
			if(auto* pat = emitter_->Pattern()) pat->Advance(dt);
			BulletEmitterContext cxt{};
			cxt.origin	  = GetCenterPos();
			cxt.targetPos = playerTransform_ ? playerTransform_->GetWorldPosition() : GetWorldPosition();
			emitter_->Update(dt, cxt);
		}
		return;
	}

	// Dying → Dead
	if(deathState_ == DeathState::Dying) {
		deathTimer_ += dt;
		float t					 = std::clamp(deathTimer_ / deathLength_, 0.0f, 1.0f);
		float rad				 = std::numbers::pi_v<float> * 0.5f * t;
		worldTransform_.rotation = Quaternion::MakeRotateAxisQuaternion(deathRotateAxis_, rad);

		// タイマー経過でも Dead へ進める
		if(t >= 1.0f ) {
			deathState_ = DeathState::Dead;
			deathTimer_ = 0.0f;
		}
		return;
	}

	if(deathState_ == DeathState::Dead) {
		Die(); // スコア付与
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
		hitFx_->PlayAll();
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
	sched.shotsPerSec = 1.5f;
	sched.useBurst = false;

	BulletEmitterConfig cfg;
	cfg.tag = "enemy_homing";

	// ここでは“パターンなし”で作る（SetPatternで後から差し込む）
	emitter_ = std::make_unique<BulletEmitter>(
		cfg, std::move(sink), std::move(aim), nullptr, sched);

	EnsurePatternBound();
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
	if(camAnchor_.z <= 0.0f) camAnchor_ = {0, 0, 55}; // 視距離の初期値

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