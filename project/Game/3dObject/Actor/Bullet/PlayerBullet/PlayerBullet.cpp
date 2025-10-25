#include "PlayerBullet.h"

#include <Engine/Foundation/Utility/FileSystem/ConfigPathResolver/ConfigPathResolver.h>
#include <Engine/Objects/Collider/SphereCollider.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utility/SceneUtility.h>

#include <iostream>

PlayerBullet::PlayerBullet(const std::string& modelName, const std::string& name)
	:BaseBullet::BaseBullet(modelName, name){

	trailFx_ = SceneAPI::Instantiate<ParticleSystemObject>("playerBulletTrail");
	trailFx_->LoadConfig("Resources/Assets/Configs/Effect/playerBulletTrail.json");

	shootFx_ = SceneAPI::Instantiate<ParticleSystemObject>("shootFx");
	shootFx_->LoadConfig("Resources/Assets/Configs/Effect/ShootFx.json");

	// collider初期化
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	collider_->SetType(ColliderType::Type_PlayerAttack);
	collider_->SetTargetType(ColliderType::Type_Enemy);
	collider_->SetOwner(this);
	collider_->SetIsDrawCollider(false);
	auto* sphereCollider = dynamic_cast<SphereCollider*>(collider_.get());
	sphereCollider->SetRadius(1.5f);

	moveSpeed_ *= 1.5f;
}

PlayerBullet::~PlayerBullet() = default;

void PlayerBullet::Initialize(){
	auto self = shared_from_this();
	trailFx_->SetParent(self);
	shootFx_->SetParent(self);
	shootFx_->Stop();
}

void PlayerBullet::OnShot() {
	shootFx_->Play();
}


