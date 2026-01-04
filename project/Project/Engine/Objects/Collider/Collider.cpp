#include "Collider.h"

#include <Engine/Collision/CollisionManager.h>
#include <Data/Engine/Configs/Scene/Objects/Collider/ColliderConfig.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>

#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <externals/imgui/imgui.h>

Collider::Collider(bool isEnuble){
	isCollisionEnabled_ = isEnuble;
	if (isCollisionEnabled_){
		CollisionManager::GetInstance()->Register(this);
	}
}

Collider::~Collider() {
	if (isCollisionEnabled_) {
		CollisionManager::GetInstance()->Unregister(this);
	}
}

void Collider::ShowGui(){
	bool enabled = isCollisionEnabled_;
	if (GuiCmd::CheckBox("Enable Collision", enabled)){
		SetCollisionEnabled(enabled);
	}

	if (!isCollisionEnabled_) return;

	GuiCmd::CheckBox("Draw Collider", isDraw_);
	GuiCmd::ColorEdit4("Collider Color", color_);
}

void Collider::ShowGui(ColliderConfig& config){

	if (ImGui::CollapsingHeader("Collider")){

		GuiCmd::DragFloat3("offset", offset_);

		bool enabled = config.isCollisionEnabled;
		if (GuiCmd::CheckBox("Enable Collision", enabled)){
			SetCollisionEnabled(enabled);
		}

		if (!config.isCollisionEnabled) return;

		GuiCmd::CheckBox("Draw Collider", config.isDraw);
		GuiCmd::ColorEdit4("Collider Color", color_);
	}
}

void Collider::NotifyCollisionEnter(Collider* other) {
	if (owner_) {
		owner_->OnCollisionEnter(other);
	}
}

void Collider::NotifyCollisionStay(Collider* other) {
	if (owner_) {
		owner_->OnCollisionStay(other);
	}
}

void Collider::NotifyCollisionExit(Collider* other) {
	if (owner_) {
		owner_->OnCollisionExit(other);
	}
}

void Collider::ApplyConfig(const ColliderConfig& config){
	isCollisionEnabled_ = config.isCollisionEnabled;
	isDraw_ = config.isDraw;
	type_ = static_cast< ColliderType >(config.colliderType);
	targetType_ = static_cast< ColliderType >(config.targetType);
}

ColliderConfig Collider::ExtractConfig() const{
	ColliderConfig config;
	config.isCollisionEnabled = isCollisionEnabled_;
	config.isDraw = isDraw_;
	config.colliderType = static_cast< int >(type_);
	config.targetType = static_cast< int >(targetType_);
	return config;
}


void Collider::SetCollisionEnabled(bool enable){
	if (isCollisionEnabled_ == enable) return; // 状態が変わらないなら何もしない

	isCollisionEnabled_ = enable;

	if (enable){
		CollisionManager::GetInstance()->Register(this);
	} else{
		CollisionManager::GetInstance()->Unregister(this);
	}
}