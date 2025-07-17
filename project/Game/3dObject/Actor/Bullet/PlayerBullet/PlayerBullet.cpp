#include "PlayerBullet.h"

#include <Engine/Application/Effects/Intermediary/FxIntermediary.h>
#include <Engine/Foundation/Utility/FileSystem/ConfigPathResolver/ConfigPathResolver.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Scene/Context/SceneContext.h>

PlayerBullet::PlayerBullet(const std::string& modelName, const std::string& name)
:BaseBullet::BaseBullet(modelName, name){
	collider_->SetType(ColliderType::Type_PlayerAttack);
	collider_->SetTargetType(ColliderType::Type_Enemy);

	trailFx_ = SceneAPI::Instantiate<ParticleSystemObject>("playerBulletTrail");
	trailFx_->LoadConfig("Resources/Assets/Configs/Effect/playerBulletTrail.json");

	shootFx_ = SceneAPI::Instantiate<ParticleSystemObject>("shootFx");
	shootFx_->LoadConfig("Resources/Assets/Configs/Effect/ShootFx.json");

	shootFx_->Play();
}

void PlayerBullet::Initialize(){
	auto self = shared_from_this();
	auto ctx = SceneContext::Current();

	trailFx_->SetParent(self);
	ctx->GetFxSystem()->AddEmitter(trailFx_);

	shootFx_->SetParent(self);
	ctx->GetFxSystem()->AddEmitter(shootFx_);

}

void PlayerBullet::Update(){

	BaseBullet::Update();
}

