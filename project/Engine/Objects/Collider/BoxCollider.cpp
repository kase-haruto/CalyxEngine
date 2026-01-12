#include "BoxCollider.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

// externals
#include <externals/imgui/imgui.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

// c++
#include <sstream> 

void BoxCollider::Initialize([[maybe_unused]] const CalyxMath::Vector3& size) {

	if (name_.empty()) {
		std::stringstream ss;
		ss << "box" << "_" << this; // 形状とアドレスを組み合わせ
		name_ = ss.str();
	}

	//JsonCoordinator::RegisterItem(name_, "ColliderSize", shape_.size);

	collisionShape_ = shape_;
	shape_.size = size;
}

BoxCollider::BoxCollider(bool isEnuble):
Collider::Collider(isEnuble){}

void BoxCollider::Update(const CalyxMath::Vector3& position, const CalyxMath::Quaternion& rotate) {
	// 回転込みでローカルオフセットをワールドへ
	const CalyxMath::Vector3 worldOffset =CalyxMath::Quaternion::RotateVector(offset_,rotate); // ← 回転を適用
	shape_.center = position + worldOffset;
	shape_.rotate = rotate;
}

void BoxCollider::Draw() {

#if defined(_DEBUG) || defined(DEVELOP)
	if (isDraw_ && isCollisionEnabled_) {
		PrimitiveDrawer::GetInstance()->DrawOBB(shape_.center, shape_.rotate, shape_.size, color_);
	}
#endif // DEBUG

}

void BoxCollider::ShowGui() {

	if(GuiCmd::CollapsingHeader("Collider")) {
		Collider::ShowGui();
		if (!isCollisionEnabled_) return;
		GuiCmd::DragFloat3("offset", offset_);
		GuiCmd::DragFloat3("Size", shape_.size, 0.1f, 0.0f, 10.0f);
	}
}

const CalyxMath::Vector3& BoxCollider::GetCenter() const {
	return shape_.center;
}

const std::variant<Sphere, OBB>& BoxCollider::GetCollisionShape() {
	collisionShape_ = shape_;
	return collisionShape_;
};