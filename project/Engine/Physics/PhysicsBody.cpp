#include "PhysicsBody.h"

#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <externals/imgui/imgui.h>

#include <algorithm>

void PhysicsBody::SetBodyType(PhysicsBodyType type) {
	// Dynamic以外へ変更したBodyは物理積分の対象外になるため、以前の速度を持ち越さない。
	// 再びDynamicへ戻した際に、古い速度で突然移動することを防ぐ。
	if(bodyType_ == PhysicsBodyType::Dynamic && type != PhysicsBodyType::Dynamic) {
		linearVelocity_ = CalyxEngine::Vector3::Zero();
	}

	bodyType_ = type;
}

void PhysicsBody::ShowGui() {
	if(ImGui::TreeNodeEx("PhysicsBody", ImGuiTreeNodeFlags_SpanAvailWidth)) {
		GuiCmd::CheckBox("Enable Physics Response", enabled_);

		int bodyType = static_cast<int>(bodyType_);
		const char* items[] = {"Static", "Kinematic", "Dynamic"};
		if(GuiCmd::Combo("Body Type", bodyType, items, 3)) {
			SetBodyType(static_cast<PhysicsBodyType>(bodyType));
		}

		GuiCmd::DragFloat("Pushback Ratio", pushbackRatio_, 0.01f, 0.0f, 1.0f);
		pushbackRatio_ = std::clamp(pushbackRatio_, 0.0f, 1.0f);

		// Dynamic固有の物理パラメータは、Dynamic選択中だけ表示する。
		if(bodyType_ == PhysicsBodyType::Dynamic) {
			GuiCmd::DragFloat3("Linear Velocity", linearVelocity_, 0.05f);
			GuiCmd::CheckBox("Use Gravity", useGravity_);
			GuiCmd::DragFloat("Gravity Scale", gravityScale_, 0.01f, 0.0f, 10.0f);
			GuiCmd::DragFloat("Mass", mass_, 0.05f, 0.001f, 10000.0f);
			SetMass(mass_);
		}

		ImGui::TreePop();
	}
}

void PhysicsBody::ApplyConfig(const PhysicsBodyConfig& config) {
	enabled_ = config.enabled;
	SetBodyType(static_cast<PhysicsBodyType>(std::clamp(config.bodyType, 0, 2)));
	pushbackRatio_ = std::clamp(config.pushbackRatio, 0.0f, 1.0f);
	useGravity_ = config.useGravity;
	gravityScale_ = std::clamp(config.gravityScale, 0.0f, 10.0f);
	SetMass(config.mass);
}

PhysicsBodyConfig PhysicsBody::ExtractConfig() const {
	PhysicsBodyConfig config;
	config.enabled = enabled_;
	config.bodyType = static_cast<int>(bodyType_);
	config.pushbackRatio = pushbackRatio_;
	config.useGravity = useGravity_;
	config.gravityScale = gravityScale_;
	config.mass = mass_;
	return config;
}

void PhysicsBody::IntegrateForces(const CalyxEngine::Vector3& gravity, float fixedDeltaTime) {
	// Dynamic以外は物理システムが速度を変更しない。
	if(bodyType_ != PhysicsBodyType::Dynamic) return;
	if(!useGravity_) return;
	if(fixedDeltaTime <= 0.0f) return;

	// 重力加速度を固定時間ぶん積分して線形速度へ加算する。
	linearVelocity_ += gravity * gravityScale_ * fixedDeltaTime;
}

void PhysicsBody::SetMass(float mass) {
	// 0除算と極端な速度変化を防ぐため、質量へ正の下限を設ける。
	mass_ = (std::max)(mass, 0.001f);
	inverseMass_ = 1.0f / mass_;
}

float PhysicsBody::GetInverseMass() const {
	// StaticとKinematicはSolverから動かさないため、無限質量を表す0を返す。
	if(bodyType_ != PhysicsBodyType::Dynamic) return 0.0f;
	return inverseMass_;
}
