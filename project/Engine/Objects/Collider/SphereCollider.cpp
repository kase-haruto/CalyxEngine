#include "SphereCollider.h"

#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

#include <externals/imgui/imgui.h>

#include <sstream>

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor
/////////////////////////////////////////////////////////////////////////////////////////
SphereCollider::SphereCollider(bool isEnuble) : Collider::Collider(isEnuble) {}

void SphereCollider::Initialize(float radius) {

	std::stringstream ss;
	ss << "sphere" << "_" << this; // 形状とアドレスを組み合わせ
	name_ = ss.str();

	collisionShape_ = Sphere{shape_};
	shape_.radius	= radius;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void SphereCollider::Update(const CxMath::Vector3& position, [[maybe_unused]] const CxMath::Quaternion& rotate) {
	// 位置を更新
	shape_.center = position + offset_;
}

void SphereCollider::Draw() {

#if defined(_DEBUG) || defined(DEVELOP)
	// 形状の描画
	if(isDraw_ && isCollisionEnabled_) {
		PrimitiveDrawer::GetInstance()->DrawSphere(shape_.center, shape_.radius, 10, color_);
	}
#endif // DEBUG
}

/////////////////////////////////////////////////////////////////////////////////////////
//		debug ui
/////////////////////////////////////////////////////////////////////////////////////////
void SphereCollider::ShowGui() {
	if(ImGui::CollapsingHeader("Collider")) {
		Collider::ShowGui();
		if(!isCollisionEnabled_) return;
		GuiCmd::DragFloat3("Offset", offset_);
		GuiCmd::DragFloat("Radius", shape_.radius, 0.1f, 0.0f, 10.0f);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		中心座標を返す
/////////////////////////////////////////////////////////////////////////////////////////
const CxMath::Vector3& SphereCollider::GetCenter() const {
	return shape_.center;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		形状を返す
/////////////////////////////////////////////////////////////////////////////////////////
const std::variant<Sphere, OBB>& SphereCollider::GetCollisionShape() {
	collisionShape_ = shape_;
	return collisionShape_;
};
