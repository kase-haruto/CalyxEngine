#include "Actor.h"

#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Objects/Collider/SphereCollider.h>

Actor::Actor(const std::string& modelName,
			 std::optional<std::string> objectName) :
	BaseGameObject::BaseGameObject(modelName, objectName) {

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