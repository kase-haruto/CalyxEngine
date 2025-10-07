#include "Enemy.h"

#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <numbers>

#include <Game/Battle/Shooting/ShootingController/EnemyShootingControllerSink.h>
#include <Game/Battle/Shooting/Details/AimProvider.h>
#include <Game/Battle/Shooting/Pattern/PatternSweepFan.h>
#include <Game/Battle/Shooting/Pattern/PatternCircleRing.h>
#include <Game/Battle/Shooting/Details/FireScheduler.h>

/* ========================================================================
/* include space
/* ===================================================================== */

/////////////////////////////////////////////////////////////////////////////////////////
//      コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
Enemy::Enemy(const std::string& modelName, const std::string objName)
	: Actor(modelName, objName) {
	worldTransform_.scale = { 2, 2, 2 };

	moveSpeed_ = Random::Generate<float>(1.0f, 3.0f);
	velocity_ = Random::GenerateVector3(-1.0f, 1.0f);

	collider_->SetType(ColliderType::Type_Enemy);
	collider_->SetTargetType(ColliderType::Type_PlayerAttack);
	collider_->SetOwner(this);
	if (auto* box = dynamic_cast<BoxCollider*>(collider_.get())) { box->SetSize({ 3, 3, 3 }); }
	collider_->SetIsDrawCollider(false);

	life_ = 1;
	waveAmplitude_ = 2.0f;
	waveSpeed_ = Random::Generate<float>(1.0f, 3.0f);

	hitFx_ = SceneAPI::Instantiate<ParticleSystemObject>("hitFx");
	hitFx_->LoadConfig("Resources/Assets/Configs/Effect/HitFx.json");

	explosionFx_ = SceneAPI::Instantiate<ParticleSystemObject>("explosionFx");
	explosionFx_->LoadConfig("Resources/Assets/Configs/Effect/Explosion.json");

	// --- スプライン追従の既定値 ---
	mover_.SetWorldSpeed(12.0f); // 等速（m/s）
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
	if (hasRoute_) {
		mover_.SetTargetTransform(playerTransform_);
	}

	EnsurePatternBound();
}

/////////////////////////////////////////////////////////////////////////////////////////
//      変換
/////////////////////////////////////////////////////////////////////////////////////////
static float Deg2Rad(float d) { return d * std::numbers::pi_v<float> / 180.0f; }

////////////////////////////////////////////////////////////////
//  Update
////////////////////////////////////////////////////////////////
void Enemy::Update(float dt) {
	/* =============================================
	    生存中のロジック
	   =============================================*/
	if (deathState_ == DeathState::Alive) {
		if (life_ <= 0) {
			// ---- 死亡フラグ立った瞬間 ----
			deathState_ = DeathState::Dying;
			explosionFx_->Play();
			deathTimer_ = 0.0f;
			deathRotateAxis_ = { 1, 0, 0 }; // 前方に倒れる
			return; // このフレームはここで終了
		}

		// 一度だけエミッタを生成（依存が揃った最初のフレーム）
		BuildEmitterIfReady();

		// =========================
		//   スプライン追従による移動
		// =========================
		if (hasRoute_) {
			// プレイヤー参照が後から入るケースに対応
			mover_.SetTargetTransform(playerTransform_);

			// スプライン上を等速で進める（アンカー適用は mover_ 側）
			mover_.Update(dt);

			// 基本位置：スプライン上の位置
			const Vector3 basePos = mover_.GetPosition();

			// 波移動（ワールドYに揺らぎを加算）
			waveTime_ += dt * waveSpeed_;
			const float offsetY = std::sin(waveTime_) * waveAmplitude_;
			worldTransform_.translation = basePos + Vector3{ 0, offsetY, 0 };

			// 方向合わせ（プレイヤーへ）
			const Vector3 myPos = worldTransform_.translation;
			const Vector3 targetPos = playerTransform_ ? playerTransform_->GetWorldPosition() : myPos;
			Vector3 d = targetPos - myPos;
			if (d.LengthSquared() > 1e-12f) {
				d = d.Normalize();
				const float yaw = std::atan2(d.x, d.z);                                  // 水平旋回
				const float pitch = std::atan2(-d.y, std::sqrt(d.x * d.x + d.z * d.z));    // 上下（LH）
				worldTransform_.rotation = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
			}
		} else {
			// フォールバック：従来の波移動（経路未設定時のみ）
			waveTime_ += dt * waveSpeed_;
			const float offsetY = std::sin(waveTime_) * waveAmplitude_;
			worldTransform_.translation = basePosition_ + Vector3{ 0, offsetY, 0 };

			// 方向合わせ（プレイヤーへ）
			const Vector3 myPos = GetWorldPosition();
			const Vector3 targetPos = playerTransform_ ? playerTransform_->GetWorldPosition() : myPos;
			Vector3 d = targetPos - myPos;
			if (d.LengthSquared() > 1e-12f) {
				d = d.Normalize();
				const float yaw = std::atan2(d.x, d.z);
				const float pitch = std::atan2(-d.y, std::sqrt(d.x * d.x + d.z * d.z));
				worldTransform_.rotation = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
			}
		}

		// 下流コントローラの更新
		if (shootingController_) {
			shootingController_->SetGameplayEngaged(this->IsGameplayEngaged());
			shootingController_->Update(dt);
		}

		// 弾幕駆動：emitter_ は一度生成したら以降は再利用
		if (this->IsGameplayEngaged() && emitter_){
			if (auto* pat = emitter_->Pattern()){
				pat->Advance(dt); 
			}

			BulletEmitterContext cxt {};
			cxt.origin = GetCenterPos(); // 先に移動を済ませてあるので常に最新
			cxt.targetPos = playerTransform_ ? playerTransform_->GetWorldPosition() : GetWorldPosition();

			emitter_->Update(dt, cxt);
		}

		return;
	}

	/* =============================================
	    倒れ演出中 (Dying)
	   =============================================*/
	if (deathState_ == DeathState::Dying) {
		deathTimer_ += dt;
		float t = std::clamp(deathTimer_ / deathLength_, 0.0f, 1.0f);

		// 0→90° まで補間して倒れる
		float rad = Deg2Rad(90.0f * t);
		worldTransform_.rotation =
			Quaternion::MakeRotateAxisQuaternion(deathRotateAxis_, rad);

		// 倒れ演出中は移動しない（位置固定）
		// worldTransform_.translation = basePosition_;

		// 演出が終わり、爆発も再生終了したら Dead へ
		if (t >= 1.0f && !explosionFx_->IsPlaying()) {
			deathState_ = DeathState::Dead;
			deathTimer_ = 0.0f;
		}
		return;
	}

	/* =============================================
	   完全に死亡
	   =============================================*/
	if (deathState_ == DeathState::Dead) {
		isAlive_ = false;
		return;
	}
}

void Enemy::EnsurePatternBound(){
	if (!emitter_) return;

	if (!pattern_ || patternKind_ != lastPatternKind_){
		pattern_ = CreatePattern(patternKind_);
		lastPatternKind_ = patternKind_;
	}
	emitter_->SetPattern(pattern_.get()); // 非所有参照を差し替え
}

////////////////////////////////////////////////////////////////
//  衝突
////////////////////////////////////////////////////////////////
void Enemy::OnCollisionEnter(Collider* other) {
	if (!other) return;
	if (collider_->GetTargetType() != other->GetType()) return;

	if (life_ >= 1) {
		life_--;
		hitFx_->Play();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//      中心座標取得
/////////////////////////////////////////////////////////////////////////////////////////
const Vector3 Enemy::GetCenterPos() const {
	const Vector3 offset = { 0.0f, 1.5f, 0.0f };
	Vector3 worldPos = Vector3::Transform(offset, worldTransform_.matrix.world);
	return worldPos;
}

BulletContainer* Enemy::GetBulletContainer() {
	return shootingController_ ? shootingController_->GetbulletContaienr() : nullptr;
}

const BulletContainer* Enemy::GetBulletContainer() const {
	return shootingController_ ? shootingController_->GetbulletContaienr() : nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////
//      親の設定
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::SetParent(WorldTransform* parent) {
	worldTransform_.parent = parent;
	basePosition_ = worldTransform_.translation;
}

/////////////////////////////////////////////////////////////////////////////////////////
//      弾コントロール
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::SetShootingController(std::unique_ptr<EnemyShootingController> controller) {
	shootingController_ = std::move(controller);
}

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
	Vector3 dir = Vector3(playerTransform_->GetWorldPosition() - myPos).Normalize();
	shootingController_->RequestShoot(GetCenterPos(), dir);
}

/////////////////////////////////////////////////////////////////////////////////////////
//      エミッタの一度だけ生成する関数
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::BuildEmitterIfReady(){
	if (emitter_) return;
	if (!shootingController_) return;
	if (!playerTransform_) return;

	auto sink = std::make_unique<EnemyShootingControllerSink>(shootingController_.get());
	auto aim = std::make_unique<AimAtTarget>();

	FireScheduler sched;
	sched.shotsPerSec = 6.0f;

	BulletEmitterConfig cfg;
	cfg.tag = "enemy_homing";

	// ここでは“パターンなし”で作る（SetPatternで後から差し込む）
	emitter_ = std::make_unique<BulletEmitter>(
		cfg, std::move(sink), std::move(aim), nullptr, sched
	);

	EnsurePatternBound();
}