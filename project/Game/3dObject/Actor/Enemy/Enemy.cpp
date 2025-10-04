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
    velocity_  = Random::GenerateVector3(-1.0f, 1.0f);

    collider_->SetType(ColliderType::Type_Enemy);
    collider_->SetTargetType(ColliderType::Type_PlayerAttack);
    collider_->SetOwner(this);
    if (auto* box = dynamic_cast<BoxCollider*>(collider_.get())) { box->SetSize({ 3, 3, 3 }); }
    collider_->SetIsDrawCollider(false);

    life_ = 1;
    waveAmplitude_ = 2.0f;
    waveSpeed_     = Random::Generate<float>(1.0f, 3.0f);

    hitFx_ = SceneAPI::Instantiate<ParticleSystemObject>("hitFx");
    hitFx_->LoadConfig("Resources/Assets/Configs/Effect/HitFx.json");

    explosionFx_ = SceneAPI::Instantiate<ParticleSystemObject>("explosionFx");
    explosionFx_->LoadConfig("Resources/Assets/Configs/Effect/Explosion.json");
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
    // ここでは emitter_ を作らず、Update() 内で BuildEmitterIfReady_() を呼びます。
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
       1. 生存中のロジック
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

        // 下流コントローラの更新
        if (shootingController_) {
            shootingController_->SetGameplayEngaged(this->IsGameplayEngaged());

            // 従来の単発発射は停止（弾幕は emitter_ が担当）
            // if (this->IsGameplayEngaged()) { Shoot(); }

            shootingController_->Update(dt);
        }

        // 弾幕駆動：emitter_ は一度生成したら以降は再利用
        if (this->IsGameplayEngaged() && emitter_) {
            if (auto* sweep = dynamic_cast<PatternSweepFan*>(emitter_->Pattern())) {
                sweep->Advance(dt); // 中心角の往復
            }

            BulletEmitterContext cxt{};
            cxt.origin      = GetCenterPos();
            // cxt.selfForward は未使用ならデフォルトのまま
            if (playerTransform_) {
                cxt.targetPos = playerTransform_->GetWorldPosition();
            } else {
                cxt.targetPos = cxt.origin;
            }
            emitter_->Update(dt, cxt);
        }

        // 方向合わせ（プレイヤーへ）
        {
            const Vector3 myPos     = GetWorldPosition();
            const Vector3 targetPos = playerTransform_ ? playerTransform_->GetWorldPosition() : myPos;

            Vector3 d = targetPos - myPos;
            if (d.LengthSquared() > 1e-12f) {
                d = d.Normalize();

                const float yaw   = std::atan2(d.x, d.z);                               // 水平旋回
                const float pitch = std::atan2(-d.y, std::sqrt(d.x * d.x + d.z * d.z)); // 上下（LH）

                const Quaternion qWorld = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
                worldTransform_.rotation = qWorld;
            }
        }

        // 波移動
        waveTime_ += dt * waveSpeed_;
        float offsetY = std::sin(waveTime_) * waveAmplitude_;
        worldTransform_.translation = basePosition_ + Vector3{ 0, offsetY, 0 };
        return;
    }

    /* =============================================
       2. 倒れ演出中 (Dying)
       =============================================*/
    if (deathState_ == DeathState::Dying) {
        deathTimer_ += dt;
        float t = std::clamp(deathTimer_ / deathLength_, 0.0f, 1.0f);

        // 0→90° まで補間して倒れる
        float rad = Deg2Rad(90.0f * t);
        worldTransform_.rotation =
            Quaternion::MakeRotateAxisQuaternion(deathRotateAxis_, rad);

        worldTransform_.translation = basePosition_; // 移動しない

        // 演出が終わり、爆発も再生終了したら Dead へ
        if (t >= 1.0f && !explosionFx_->IsPlaying()) {
            deathState_ = DeathState::Dead;
            deathTimer_ = 0.0f;
        }
        return;
    }

    /* =============================================
       3. 完全に死亡 (Dead)
       =============================================*/
    if (deathState_ == DeathState::Dead) {
        isAlive_ = false;
        return;
    }
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

void Enemy::SetPlayerTransform(const WorldTransform* tf) { playerTransform_ = tf; }

/////////////////////////////////////////////////////////////////////////////////////////
//      移動
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::Move() {}

/////////////////////////////////////////////////////////////////////////////////////////
//      弾発射（従来：単発）→ 弾幕化後は呼ばない（互換用に残置）
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::Shoot() {
    Vector3 myPos = GetCenterPos();
    Vector3 dir = Vector3(playerTransform_->GetWorldPosition() - myPos).Normalize();
    shootingController_->RequestShoot(GetCenterPos(), dir);
}

/////////////////////////////////////////////////////////////////////////////////////////
//      ★ エミッタの一度だけ生成する関数
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::BuildEmitterIfReady() {
    if (emitter_) return;                 // 既に作成済み
    if (!shootingController_) return;     // 下流がないと撃てない
    if (!playerTransform_) return;        // 目標がないと狙えない

    // Sink：既存コントローラへまとめて流す
    auto sink = std::make_unique<EnemyShootingControllerSink>(shootingController_.get());

    // Aim：プレイヤー狙い
    auto aim  = std::make_unique<AimAtTarget>();

    // Pattern：左右に揺れる N-way
    auto pattern = std::make_unique<PatternSweepFan>();
    pattern->nWay         = 9;
    pattern->spreadDeg    = 50.0f;
    pattern->periodSec    = 2.5f;
    pattern->amplitudeDeg = 40.0f;

    // Scheduler：1秒あたりのトリガ回数
    FireScheduler sched;
    sched.shotsPerSec = 6.0f;
    // バーストさせたい場合：
    // sched.useBurst = true;
    // sched.burstsPerTrigger = 5;
    // sched.burstIntervalSec = 0.07f;

    BulletEmitterConfig cfg;
    cfg.tag = "enemy_homing";
    // cfg.shotSpeed = 0.0f; // 弾側で速度管理なら未使用

    emitter_ = std::make_unique<BulletEmitter>(
        cfg, std::move(sink), std::move(aim), std::move(pattern), sched
    );
}
