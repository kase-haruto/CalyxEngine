#include "SphereCollider.h"
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

#include <externals/imgui/imgui.h>

#include <sstream> 

SphereCollider::SphereCollider(bool isEnuble) :
	Collider::Collider(isEnuble) {}

void SphereCollider::Initialize(float radius){

	std::stringstream ss;
	ss << "sphere" << "_" << this; // 形状とアドレスを組み合わせ
	name_ = ss.str();

	collisionShape_ = Sphere {shape_};
	shape_.radius = radius;

}

void SphereCollider::Update(const Vector3& position,[[maybe_unused]] const Quaternion& rotate){
	// 位置を更新
	shape_.center = position;
}

void SphereCollider::Draw(){

#ifdef _DEBUG
	if (isDraw_ && isCollisionEnabled_) {
		PrimitiveDrawer::GetInstance()->DrawSphere(shape_.center, shape_.radius, 10, color_);
	}
#endif // DEBUG

}

void SphereCollider::ShowGui(){
	if(ImGui::CollapsingHeader("Collider")) {
		Collider::ShowGui();
		if (!isCollisionEnabled_) return;
		ImGui::DragFloat("Radius", &shape_.radius, 0.1f, 0.0f, 10.0f);
	}
}

const Vector3& SphereCollider::GetCenter() const{
	return shape_.center;
}

const std::variant<Sphere, OBB>& SphereCollider::GetCollisionShape(){
	collisionShape_ = shape_;
	return collisionShape_;
};
