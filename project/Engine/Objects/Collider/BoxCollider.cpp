#include "BoxCollider.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Data/Engine/Configs/Scene/Objects/Collider/ColliderConfig.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

// externals
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <externals/imgui/imgui.h>

// c++
#include <sstream>

void BoxCollider::Initialize([[maybe_unused]] const CalyxMath::Vector3& size) {

	if(name_.empty()) {
		std::stringstream ss;
		ss << "box" << "_" << this; // 形状とアドレスを組み合わせ
		name_ = ss.str();
	}

	// JsonCoordinator::RegisterItem(name_, "ColliderSize", shape_.size);

	collisionShape_ = shape_;
	shape_.size		= size;
}

BoxCollider::BoxCollider(bool isEnuble) : Collider::Collider(isEnuble) {}

void BoxCollider::Update(const CalyxMath::Vector3& position, const CalyxMath::Quaternion& rotate) {
	// 回転込みでローカルオフセットをワールドへ
	const CalyxMath::Vector3 worldOffset = CalyxMath::Quaternion::RotateVector(offset_, rotate);
	shape_.center						 = position + worldOffset;

	// 親の回転に自身の回転オフセットを掛け合わせる
	CalyxMath::Quaternion localRot = CalyxMath::Quaternion::EulerToQuaternion(rotateOffset_);
	shape_.rotate				   = rotate * localRot;
}

void BoxCollider::Draw() {

#if defined(_DEBUG) || defined(DEVELOP)
	if(isDraw_ && isCollisionEnabled_) {
		PrimitiveDrawer::GetInstance()->DrawOBB(shape_.center, shape_.rotate, shape_.size, color_);
	}
#endif // DEBUG
}

void BoxCollider::ShowGui() {
	if(ImGui::TreeNodeEx("BoxCollider", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
		Collider::ShowGui();
		if(isCollisionEnabled_) {
			GuiCmd::DragFloat3("Size", shape_.size, 0.1f, 0.0f, 1000.0f);
		}
		ImGui::TreePop();
	}
}

const CalyxMath::Vector3& BoxCollider::GetCenter() const { return shape_.center; }

const std::variant<Sphere, OBB>& BoxCollider::GetCollisionShape() {
	collisionShape_ = shape_;
	return collisionShape_;
};

void BoxCollider::ApplyConfig(const ColliderConfig& config) {
	Collider::ApplyConfig(config);
	shape_.size = config.size;
}

ColliderConfig BoxCollider::ExtractConfig() const {
	ColliderConfig config = Collider::ExtractConfig();
	config.size			  = shape_.size;
	return config;
}