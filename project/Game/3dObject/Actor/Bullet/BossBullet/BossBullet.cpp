#include "BossBullet.h"
/* ========================================================================
/*   include space
/* ===================================================================== */
// Engine
#include <Engine/Scene/Utility/SceneUtility.h>

// c++
#include <iostream>
#include <Engine\Objects\Collider\SphereCollider.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
BossBullet::BossBullet(const std::string& modelName, const std::string& name) :BaseBullet::BaseBullet(modelName, name) {


	// fx
	shootFx_ = SceneAPI::Instantiate<CalyxEffect::FxObject>("BossBulletTrail");
	auto fx = shootFx_.lock();
	fx->LoadFromPath("Effect/BossBulletTrail");

	// collider 初期化
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	collider_->SetType(ColliderType::Type_EnemyAttack);
	collider_->SetTargetType(ColliderType::Type_Player);
	collider_->SetOwner(this);
	collider_->SetIsDrawCollider(false);
	auto* sphereCollider = dynamic_cast<SphereCollider*>(collider_.get());
	sphereCollider->SetRadius(2.5f);

	moveSpeed_ *= 1.5f;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
BossBullet::~BossBullet() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void BossBullet::Initialize() {
	auto self = shared_from_this();
	auto fx = shootFx_.lock();
	fx->SetParent(self);
	fx->StopAll();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		発射時処理
/////////////////////////////////////////////////////////////////////////////////////////
void BossBullet::OnShot() {
	auto fx = shootFx_.lock();
	fx->PlayAll();
}