#include "EnemyBullet.h"
/* ========================================================================
/*   include space
/* ===================================================================== */
// engine
#include "Engine/Objects/3D/Actor/BaseGameObject.h"

#include <Engine/Scene/Utility/SceneUtility.h>


static inline Vector3 SafeNormalize(const Vector3& v, const Vector3& fb = {0, 0, 1}) {
	float L = v.Length();
	return (L > 1e-6f) ? (v / L) : fb;
}

static inline Vector3 TurnTowards(const Vector3& fromDir, const Vector3& toDir, float maxTurnRad) {
	Vector3 f = SafeNormalize(fromDir);
	Vector3 t = SafeNormalize(toDir, f);
	float d = std::clamp(Vector3::Dot(f, t), -1.0f, 1.0f);
	float ang = std::acos(d);
	if (ang < 1e-5f) return t;
	float k = (std::min)(1.0f, maxTurnRad / ang);
	return SafeNormalize(f * (1.0f - k) + t * k, t);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
EnemyBullet::EnemyBullet(const std::string& modelName, const std::string& name)
	: BaseBullet::BaseBullet(modelName, name) {

	collider_->SetType(ColliderType::Type_EnemyAttack);
	collider_->SetTargetType(ColliderType::Type_Player);

	trailFx_ = SceneAPI::Instantiate<FxObject>("TrailFx");
	auto fx = trailFx_.lock();
	fx->LoadFromPath("Effect/EnemyBulletTrailEffect");
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
EnemyBullet::~EnemyBullet() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyBullet::Initialize() {
	this->SetDrawEnable(true);

	auto self = shared_from_this();
	auto fx = trailFx_.lock();
	fx->SetParent(self);
	fx->StopAll();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		発射時処理
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyBullet::OnShot() {
	auto fx = trailFx_.lock();
	fx->PlayAll();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyBullet::Update(float dt) {
	age_ += dt;

	// ---- 軽ホーミング（必要時間帯のみ）----
	if (homing_.enable && age_ >= homing_.startDelaySec && age_ <= homing_.endTimeSec)
	{
		// ターゲットが設定されていなければ早期リターン
		if (wTarget_.expired()) { return; }

		if (auto tgt = wTarget_.lock())
		{
			const Vector3 myPos = GetWorldTransform().GetWorldPosition();
			const Vector3 tgtPos = tgt->GetWorldTransform().GetWorldPosition();
			const Vector3 toTarget = tgtPos - myPos;

			Vector3 v = GetVelocity();
			float sp = v.Length();
			if (sp > 1e-5f)
			{
				const float maxTurn = homing_.turnRateRadPerSec * dt;
				Vector3 newDir = TurnTowards(v / sp, toTarget, maxTurn);
				Vector3 desiredVel = newDir * sp;

				if (homing_.damping > 0.0f)
				{
					float a = 1.0f - std::exp(-homing_.damping * dt);
					v = v * (1.0f - a) + desiredVel * a;
				} else { v = desiredVel; }
				SetVelocity(v);
			}
		}
	}

	BaseBullet::Update(dt);
}


/////////////////////////////////////////////////////////////////////////////////////////
//		
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyBullet::direct2Camera() {
	//カメラの方向にビルボードさせる
}