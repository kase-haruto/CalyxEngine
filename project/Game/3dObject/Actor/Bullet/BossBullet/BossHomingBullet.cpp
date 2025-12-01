#include "BossHomingBullet.h"
/* ========================================================================
/*   include space
/* ===================================================================== */
// Engine
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

///////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
///////////////////////////////////////////////////////////////////////////////////////////
BossHomingBullet::BossHomingBullet(const std::string& modelName,
								   const std::string& name) : HomingBullet::HomingBullet(modelName, name) {
	collider_->SetType(ColliderType::Type_EnemyAttack);
	collider_->SetTargetType(ColliderType::Type_Player);
	collider_->SetOwner(this);
	collider_->SetIsDrawCollider(false);
	auto* boxCollider = dynamic_cast<SphereCollider*>(collider_.get());
	boxCollider->SetRadius(2.5f);

	trailFx_ = SceneAPI::Instantiate<FxObject>("BossHomingBulletTrail");
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
	if (homingTimer_ <= homingDelay_ || homingTimer_ >= homingLimitTime_) {
		// ホーミング処理なし → 直進だけ
		BaseBullet::Update(dt);
		return;
	}

	// ホーミング開始後
	HomingBullet::Update(dt);
}
/////////////////////////////////////////////////////////////////////////////////////////
//		imgui
/////////////////////////////////////////////////////////////////////////////////////////
void BossHomingBullet::DerivativeGui() {
	HomingBullet::DerivativeGui();
	GuiCmd::DragFloat("homing Delay", homingDelay_, 0.1f, 0.0f, 10.0f);
	GuiCmd::DragFloat("homing time", homingTimer_, 0.1f, 0.0f, 10.0f);
}
