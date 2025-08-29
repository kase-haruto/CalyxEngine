#include "EnemyBullet.h"
/* ========================================================================
/*   include space
/* ===================================================================== */
// engine
#include <Engine/Scene/Utility/SceneUtility.h>

// c++
#include <iostream>

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
EnemyBullet::EnemyBullet(const std::string& modelName, const std::string& name)
	:BaseBullet::BaseBullet(modelName, name){
	collider_->SetType(ColliderType::Type_EnemyAttack);
	collider_->SetTargetType(ColliderType::Type_Player);

	trailFx_ = SceneAPI::Instantiate<ParticleSystemObject>("playerBulletTrail");
	trailFx_->LoadConfig("Resources/Assets/Configs/Effect/playerBulletTrail.json");

	shootFx_ = SceneAPI::Instantiate<ParticleSystemObject>("shootFx");
	shootFx_->LoadConfig("Resources/Assets/Configs/Effect/ShootFx.json");

}

/////////////////////////////////////////////////////////////////////////////////////////
//		デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
EnemyBullet::~EnemyBullet(){
	auto ctx = SceneContext::Current();
	ctx->RemoveEditorObject(trailFx_);
	ctx->RemoveEditorObject(shootFx_);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyBullet::Initialize(){
	auto self = shared_from_this();
	trailFx_->SetParent(self);
	shootFx_->SetParent(self);
	shootFx_->Stop();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		発射時処理
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyBullet::OnShot() {
	shootFx_->Play();
}
