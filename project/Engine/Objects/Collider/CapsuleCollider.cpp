#include "CapsuleCollider.h"

#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"
#include <Data/Engine/Configs/Scene/Objects/Collider/ColliderConfig.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

#include <externals/imgui/imgui.h>

#include <algorithm>
#include <sstream>

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
CapsuleCollider::CapsuleCollider(bool isEnuble) : Collider::Collider(isEnuble) {}

void CapsuleCollider::Initialize(float radius, float height) {
	if(name_.empty()) {
		std::stringstream ss;
		ss << "capsule" << "_" << this;
		name_ = ss.str();
	}

	shape_.radius = (std::max)(radius, 0.0f);
	shape_.height = (std::max)(height, shape_.radius * 2.0f);
	shape_.rotate = CalyxEngine::Quaternion::MakeIdentity();
	collisionShape_ = shape_;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void CapsuleCollider::Update(const CalyxEngine::Vector3& position, const CalyxEngine::Quaternion& rotate) {
	// 回転込みでローカルオフセットをワールドへ反映する
	const CalyxEngine::Vector3 worldOffset = CalyxEngine::Quaternion::RotateVector(offset_, rotate);
	shape_.center = position + worldOffset;

	// 親の回転に自身の回転オフセットを掛け合わせる
	CalyxEngine::Quaternion localRot = CalyxEngine::Quaternion::EulerToQuaternion(rotateOffset_);
	shape_.rotate = rotate * localRot;
}

void CapsuleCollider::Draw() {
#if defined(_DEBUG) || defined(DEVELOP)
	if(isDraw_ && isCollisionEnabled_) {
		PrimitiveDrawer::GetInstance()->DrawCapsule(shape_.center, shape_.rotate, shape_.radius, shape_.height, color_);
	}
#endif // DEBUG
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デバッグ用UI
/////////////////////////////////////////////////////////////////////////////////////////
void CapsuleCollider::ShowGui() {
	if(ImGui::TreeNodeEx("CapsuleCollider", ImGuiTreeNodeFlags_SpanAvailWidth)) {
		Collider::ShowGui();
		if(isCollisionEnabled_) {
			GuiCmd::DragFloat("Radius", shape_.radius, 0.1f, 0.0f, 1000.0f);
			GuiCmd::DragFloat("Height", shape_.height, 0.1f, 0.0f, 1000.0f);
			shape_.height = (std::max)(shape_.height, shape_.radius * 2.0f);
		}
		ImGui::TreePop();
	}
}

void CapsuleCollider::ApplyConfig(const ColliderConfig& config) {
	Collider::ApplyConfig(config);
	shape_.radius = (std::max)(config.capsuleRadius, 0.0f);
	shape_.height = (std::max)(config.capsuleHeight, shape_.radius * 2.0f);
}

ColliderConfig CapsuleCollider::ExtractConfig() const {
	ColliderConfig config = Collider::ExtractConfig();
	config.capsuleRadius = shape_.radius;
	config.capsuleHeight = shape_.height;
	return config;
}

float CapsuleCollider::GetColliderRadius() const {
	const float halfSegment = (std::max)(0.0f, shape_.height * 0.5f - shape_.radius);
	return halfSegment + shape_.radius;
}

const CalyxEngine::Vector3& CapsuleCollider::GetCenter() const {
	return shape_.center;
}

const CollisionShape& CapsuleCollider::GetCollisionShape() {
	collisionShape_ = shape_;
	return collisionShape_;
};
