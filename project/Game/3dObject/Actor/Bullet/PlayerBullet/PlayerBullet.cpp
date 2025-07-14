#include "PlayerBullet.h"

#include <Engine/Application/Effects/Intermediary/FxIntermediary.h>
#include <Engine/Foundation/Utility/FileSystem/ConfigPathResolver/ConfigPathResolver.h>

PlayerBullet::PlayerBullet(const std::string& modelName, const std::string& name)
:BaseBullet::BaseBullet(modelName, name){
	collider_->SetType(ColliderType::Type_PlayerAttack);
	collider_->SetTargetType(ColliderType::Type_Enemy);

	trailFx_ = std::make_shared<ParticleSystemObject>("playerBulletTrail");
	trailFx_->LoadConfig("Resources/Assets/Configs/Effect/playerBulletTrail.json");

	shootFx_ = std::make_shared<ParticleSystemObject>("shootFx");
	shootFx_->LoadConfig("Resources/Assets/Configs/Effect/ShootFx.json");

	shootFx_->Play();
}

void PlayerBullet::Initialize(){
	auto self = shared_from_this();
	trailFx_->SetParent(self);
	FxIntermediary::GetInstance()->Attach(trailFx_);

	shootFx_->SetParent(self);
	FxIntermediary::GetInstance()->Attach(shootFx_);
}

void PlayerBullet::Update(){

	BaseBullet::Update();
}

