#include "SphereCollider.h"

#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"
#include <Data/Engine/Configs/Scene/Objects/Collider/ColliderConfig.h>
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
void SphereCollider::Update(const CalyxMath::Vector3& position, const CalyxMath::Quaternion& rotate) {
	// 回転込みでローカルオフセットをワールドへ
	const CalyxMath::Vector3 worldOffset = CalyxMath::Quaternion::RotateVector(offset_, rotate);
	shape_.center						 = position + worldOffset;
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
	if(ImGui::TreeNodeEx("SphereCollider", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
		Collider::ShowGui();
		if(isCollisionEnabled_) {
			GuiCmd::DragFloat("Radius", shape_.radius, 0.1f, 0.0f, 10.0f);
		}
		ImGui::TreePop();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		中心座標を返す
/////////////////////////////////////////////////////////////////////////////////////////
const CalyxMath::Vector3& SphereCollider::GetCenter() const {
	return shape_.center;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		形状を返す
/////////////////////////////////////////////////////////////////////////////////////////
const std::variant<Sphere, OBB>& SphereCollider::GetCollisionShape() {
	collisionShape_ = shape_;
	return collisionShape_;
};

void SphereCollider::ApplyConfig(const ColliderConfig& config) {
	Collider::ApplyConfig(config);
	shape_.radius = config.radius;
}

ColliderConfig SphereCollider::ExtractConfig() const {
	ColliderConfig config = Collider::ExtractConfig();
	config.radius		  = shape_.radius;
	return config;
}