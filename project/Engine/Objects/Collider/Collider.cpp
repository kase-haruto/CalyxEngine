#include "Collider.h"

#include "Engine/Objects/3D/Actor/BaseGameObject.h"

#include <Data/Engine/Configs/Scene/Objects/Collider/ColliderConfig.h>
#include <Engine/Collision/CollisionManager.h>

#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <externals/imgui/imgui.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
Collider::Collider(bool isEnuble) {
	isCollisionEnabled_ = isEnuble;
	if(isCollisionEnabled_) {
		CollisionManager::GetInstance()->Register(this);
	}
}

Collider::~Collider() {
	if(isCollisionEnabled_) {
		CollisionManager::GetInstance()->Unregister(this);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デバッグ用ui
/////////////////////////////////////////////////////////////////////////////////////////
void Collider::ShowGui() {
	bool enabled = isCollisionEnabled_;
	if(GuiCmd::CheckBox("Enable Collision", enabled)) {
		SetCollisionEnabled(enabled);
	}

	if(!isCollisionEnabled_) return;

	GuiCmd::CheckBox("Draw Collider", isDraw_);
	GuiCmd::ColorEdit4("Collider Color", color_);
	GuiCmd::DragFloat3("Offset", offset_);
	GuiCmd::DragFloat3("Rotate Offset", rotateOffset_);
}

void Collider::ShowGui(ColliderConfig& config) {

	if(ImGui::CollapsingHeader("Collider")) {

		GuiCmd::DragFloat3("offset", config.offset);
		GuiCmd::DragFloat3("rotate", config.rotate);

		bool enabled = config.isCollisionEnabled;
		if(GuiCmd::CheckBox("Enable Collision", enabled)) {
			SetCollisionEnabled(enabled);
		}

		if(!config.isCollisionEnabled) return;

		GuiCmd::CheckBox("Draw Collider", config.isDraw);
		GuiCmd::ColorEdit4("Collider Color", color_);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		衝突通知(衝突フレーム
/////////////////////////////////////////////////////////////////////////////////////////
void Collider::NotifyCollisionEnter(Collider* other) {
	if(onEnter_) onEnter_(other);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		衝突通知(衝突中
/////////////////////////////////////////////////////////////////////////////////////////
void Collider::NotifyCollisionStay(Collider* other) {
	if(onStay_) onStay_(other);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		衝突通知(離れた
/////////////////////////////////////////////////////////////////////////////////////////
void Collider::NotifyCollisionExit(Collider* other) {
	if(onExit_) onExit_(other);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		config適用
/////////////////////////////////////////////////////////////////////////////////////////
void Collider::ApplyConfig(const ColliderConfig& config) {
	isCollisionEnabled_ = config.isCollisionEnabled;
	isDraw_				= config.isDraw;
	type_				= static_cast<ColliderType>(config.colliderType);
	targetType_			= static_cast<ColliderType>(config.targetType);
	offset_				= config.offset;
	rotateOffset_		= config.rotate;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		config吐き出し
/////////////////////////////////////////////////////////////////////////////////////////
ColliderConfig Collider::ExtractConfig() const {
	ColliderConfig config;
	config.isCollisionEnabled = isCollisionEnabled_;
	config.isDraw			  = isDraw_;
	config.colliderType		  = static_cast<int>(type_);
	config.targetType		  = static_cast<int>(targetType_);
	config.offset			  = offset_;
	config.rotate			  = rotateOffset_;
	return config;
}

CalyxMath::Vector3 Collider::GetWorldPos() const {
	if(owner_) {
		return owner_->GetWorldPosition();
	}
	// オーナーがいない場合はゼロベクトルを返す
	return CalyxMath::Vector3::Zero();
}
/////////////////////////////////////////////////////////////////////////////////////////
//		Collisionするか
/////////////////////////////////////////////////////////////////////////////////////////
void Collider::SetCollisionEnabled(bool enable) {
	if(isCollisionEnabled_ == enable) return; // 状態が変わらないなら何もしない

	isCollisionEnabled_ = enable;

	if(enable) {
		CollisionManager::GetInstance()->Register(this);
	} else {
		CollisionManager::GetInstance()->Unregister(this);
	}
}