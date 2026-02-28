#include "BossHomingBullet.h"
/* ========================================================================
/*   include space
/* ===================================================================== */
// Engine
#include "Game/3d/GameCamera/RailCamera.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine\Objects\Collider\SphereCollider.h>

///////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
///////////////////////////////////////////////////////////////////////////////////////////
BossHomingBullet::BossHomingBullet(const std::string& modelName,
								   const std::string& name)
	: BaseEnemyHomingBullet(modelName, name) {
	paramData_.LoadParams();

	// 基底クラスの param_ ポインタを派生クラスの paramData_ に向ける
	BaseEnemyHomingBullet::param_ = &paramData_;

	collider_->SetType(ColliderType::Type_EnemyAttack);
	collider_->SetTargetType(ColliderType::Type_Player);
	collider_->SetOwner(this);
	collider_->SetIsDrawCollider(false);
	auto* boxCollider = dynamic_cast<SphereCollider*>(collider_.get());
	boxCollider->SetRadius(paramData_.collisionRadius);

	trailFx_ = SceneAPI::Instantiate<CalyxEffect::FxObject>("BossHomingBulletTrail");
	auto fx	 = trailFx_.lock();
	fx->LoadFromPath("Effect/BossBulletTrail");

	moveSpeed_ = 1.0f;
	lifeTime_  = paramData_.lifeTime;

	// モデルの描画はしない
	BaseGameObject::SetDrawEnable(false);
}

BossHomingBullet::BossHomingBullet()  = default;
BossHomingBullet::~BossHomingBullet() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
//////////////////////////////////////////////////////////////////////////////////////////
void BossHomingBullet::Update(float dt) {
	homingTimer_ += dt;

	// ホーミング開始前 または 終了後は直進
	bool isHomingPhase = (homingTimer_ > paramData_.homingDelay &&
						  homingTimer_ < paramData_.homingDelay + paramData_.homingDurationSec);

	if(isHomingPhase) {
		// ホーミング中（BaseEnemyHomingBullet::Update が内部で BaseBullet::Update を呼ぶ）
		BaseEnemyHomingBullet::Update(dt);
	} else {
		// ホーミングなし（直進）
		BaseBullet::Update(dt);
	}

	// ---- カメラ距離によるエフェクトのフェード ----
	if(auto fx = trailFx_.lock()) {
		if(auto cam = SceneContext::Current()->FindFirst<RailCamera>()) {
			// Shader側で10m〜50mのフェードを実行
			fx->SetCameraFade(10.0f, 50.0f);

			// 密度スケーリングのために AlphaMultiplier も反映
			const CalyxMath::Vector3 camPos = cam->GetTranslate();
			const CalyxMath::Vector3 myPos	= GetCenterPos();
			float					 dist	= (myPos - camPos).Length();

			float alpha = std::clamp((dist - 10.0f) / (50.0f - 10.0f), 0.0f, 1.0f);
			fx->SetAlphaMultiplier(alpha);
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
//		imgui
/////////////////////////////////////////////////////////////////////////////////////////
void BossHomingBullet::DerivativeGui() {

	// デバッグgui
	if(paramData_.ShowGui()) {
		// 値の反映
		lifeTime_ = paramData_.lifeTime;
	}
}

BossHomingBullet::BossHomingParam::BossHomingParam() {
	AddField("homingDelay", homingDelay).Category("BossHoming").Tooltip("ホーミング遅延");
	AddField("collisionRadius", collisionRadius).Category("BossHoming").Tooltip("衝突半径");
	AddField("lifeTime", lifeTime).Category("BossHoming").Tooltip("寿命");
}

CalyxEngine::ParamPath BossHomingBullet::BossHomingParam::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Game, "BossHomingBullet", GetSubRootPath()};
}