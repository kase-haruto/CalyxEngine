#include "BossBullet.h"
/* ========================================================================
/*   include space
/* ===================================================================== */
// Engine
#include <Engine/Scene/Utility/SceneUtility.h>

// c++
#include <iostream>

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
BossBullet::BossBullet(const std::string& modelName, const std::string& name) :BaseBullet::BaseBullet(modelName, name) {
	collider_->SetType(ColliderType::Type_EnemyAttack);
	collider_->SetTargetType(ColliderType::Type_Player);

	trailFx_ = SceneAPI::Instantiate<ParticleSystemObject>("playerBulletTrail");
	trailFx_->LoadConfig("Effect/playerBulletTrail");

	shootFx_ = SceneAPI::Instantiate<ParticleSystemObject>("shootFx");
	shootFx_->LoadConfig("Effect/ShootFx");
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
BossBullet::~BossBullet() {
	auto ctx = SceneContext::Current();
	ctx->RemoveEditorObject(trailFx_);
	ctx->RemoveEditorObject(shootFx_);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void BossBullet::Initialize() {
	auto self = shared_from_this();
	trailFx_->SetParent(self);
	shootFx_->SetParent(self);
	shootFx_->Stop();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		発射時処理
/////////////////////////////////////////////////////////////////////////////////////////
void BossBullet::OnShot() {
	shootFx_->Play();
}

