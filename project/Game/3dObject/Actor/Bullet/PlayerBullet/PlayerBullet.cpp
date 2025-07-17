#include "PlayerBullet.h"

#include <Engine/Foundation/Utility/FileSystem/ConfigPathResolver/ConfigPathResolver.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <iostream>

PlayerBullet::PlayerBullet(const std::string& modelName, const std::string& name)
	:BaseBullet::BaseBullet(modelName, name){
	collider_->SetType(ColliderType::Type_PlayerAttack);
	collider_->SetTargetType(ColliderType::Type_Enemy);

	trailFx_ = SceneAPI::Instantiate<ParticleSystemObject>("playerBulletTrail");
	trailFx_->LoadConfig("Resources/Assets/Configs/Effect/playerBulletTrail.json");
	std::cout << "[Create] trailFx_: " << trailFx_.get()
		<< ", GUID: " << trailFx_->GetGuid().ToString() << "\n";

	shootFx_ = SceneAPI::Instantiate<ParticleSystemObject>("shootFx");
	shootFx_->LoadConfig("Resources/Assets/Configs/Effect/ShootFx.json");

	shootFx_->Play();
}

PlayerBullet::~PlayerBullet(){
	auto ctx = SceneContext::Current();
	std::cout << "[Destroy] trailFx_: " << trailFx_.get()
		<< ", GUID: " << trailFx_->GetGuid().ToString() << "\n";
	ctx->RemoveEditorObject(trailFx_);
	ctx->RemoveEditorObject(shootFx_);
}

void PlayerBullet::Initialize(){
	auto self = shared_from_this();
	trailFx_->SetParent(self);
	shootFx_->SetParent(self);

}

void PlayerBullet::Update(){

	BaseBullet::Update();
}

