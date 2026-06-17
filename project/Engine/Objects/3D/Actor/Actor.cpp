#include "Actor.h"

#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Objects/Collider/SphereCollider.h>
#include <externals/imgui/imgui.h>

Actor::Actor() {
	physicsBody_.SetBodyType(PhysicsBodyType::Kinematic);
	config_.GetConfig().physicsBodyConfig.bodyType = static_cast<int>(PhysicsBodyType::Kinematic);
	characterMovement_.SetOwner(this);
}

Actor::Actor(const std::string& modelName,
			 std::optional<std::string> objectName) :
	BaseGameObject::BaseGameObject(modelName, objectName) {
	physicsBody_.SetBodyType(PhysicsBodyType::Kinematic);
	config_.GetConfig().physicsBodyConfig.bodyType = static_cast<int>(PhysicsBodyType::Kinematic);
	characterMovement_.SetOwner(this);
}

void Actor::AlwaysUpdate(float dt) {
	
	BaseGameObject::AlwaysUpdate(dt);
}
void Actor::Update(float dt) {
	// Actor は CharacterMovementComponent で床探索・接地補正を行ってから、
	// BaseGameObject 側で Transform / Collider / Model を通常更新する。
	characterMovement_.Tick(dt);
	BaseGameObject::Update(dt);
}

void Actor::DerivativeGui() {
	BaseGameObject::DerivativeGui();
	if(ImGui::TreeNodeEx("CharacterMovement", ImGuiTreeNodeFlags_SpanAvailWidth)) {
		characterMovement_.ShowGui();
		const FindFloorResult& floor = characterMovement_.GetCurrentFloor();
		ImGui::Text("Mode: %s", characterMovement_.IsMovingOnGround() ? "Walking" : "Falling");
		ImGui::Text("Floor Hit: %s", floor.blockingHit ? "true" : "false");
		ImGui::Text("Walkable: %s", floor.walkableFloor ? "true" : "false");
		ImGui::Text("Floor Distance: %.3f", floor.floorDistance);
		ImGui::TreePop();
	}
}

float Actor::GetCollisionRadius() const {
	if (!collider_) return 0.0f;
	if (auto s = dynamic_cast<const SphereCollider*>(collider_.get())) return s->GetColliderRadius();
	if (auto b = dynamic_cast<const BoxCollider*>(collider_.get())) {
		auto half = b->GetSize() * 0.5f;
		return std::sqrt(half.x * half.x + half.y * half.y + half.z * half.z);
	}
	return collider_->GetColliderRadius();
}
