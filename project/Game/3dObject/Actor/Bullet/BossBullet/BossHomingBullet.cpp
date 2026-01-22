#include "BossHomingBullet.h"
/* ========================================================================
/*   include space
/* ===================================================================== */
// Engine
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine\Objects\Collider\SphereCollider.h>

///////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
///////////////////////////////////////////////////////////////////////////////////////////
BossHomingBullet::BossHomingBullet(const std::string& modelName,
								   const std::string& name)
	: BaseEnemyHomingBullet::BaseEnemyHomingBullet(modelName, name) {
	collider_->SetType(ColliderType::Type_EnemyAttack);
	collider_->SetTargetType(ColliderType::Type_Player);
	collider_->SetOwner(this);
	collider_->SetIsDrawCollider(false);
	auto* boxCollider = dynamic_cast<SphereCollider*>(collider_.get());
	boxCollider->SetRadius(param_.collisionRadius);

	trailFx_ = SceneAPI::Instantiate<CalyxEffect::FxObject>("BossHomingBulletTrail");
	auto fx	 = trailFx_.lock();
	fx->LoadFromPath("Effect/BossBulletTrail");


}

BossHomingBullet::BossHomingBullet()  = default;
BossHomingBullet::~BossHomingBullet() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
//////////////////////////////////////////////////////////////////////////////////////////
void BossHomingBullet::Update(float dt) {
	homingTimer_ += dt;

	// ホーミング開始前は直進
	if (homingTimer_ <= param_.homingDelay || homingTimer_ >= param_.homingLimitTime) {
		// ホーミング処理なし から 直進だけ
		BaseBullet::Update(dt);
		return;
	}

	// ホーミング開始後
	BaseEnemyHomingBullet::Update(dt);
}
/////////////////////////////////////////////////////////////////////////////////////////
//		imgui
/////////////////////////////////////////////////////////////////////////////////////////
void BossHomingBullet::DerivativeGui() {
	param_.ShowGui();
}


BossHomingBullet::BossHomingParam::BossHomingParam() {
	AddField("homingDelay", homingDelay).Tooltip("ホーミング遅延");
	AddField("homingLimitTime", homingLimitTime).Tooltip("ホーミング時間");
	AddField("collisionRadius", collisionRadius).Tooltip("衝突半径");
}

CalyxEngine::ParamPath BossHomingBullet::BossHomingParam::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Game,"BossHomingBullet","Actor/Bullet"};
}