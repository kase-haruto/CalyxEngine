#include "DiceSocketObject.h"

#include <numbers>

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine\Foundation\Utility\Ease\CxEase.h>

REGISTER_SCENE_OBJECT(DiceSocketObject)

DiceSocketObject::DiceSocketObject(
	const std::string& modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {

	worldTransform_.translation.y += 2.5f;
}

const CalyxEngine::Vector3 DiceSocketObject::GetSocketPos() {
	CalyxEngine::Vector3 position = worldTransform_.translation;
	float scale = 0.5f;
	int index = 1 - clearCount_ + (diceSocketCount_ * 2);
	position.x += scale * static_cast<float>(index);
	return position;
}

const CalyxEngine::Quaternion DiceSocketObject::GetSameNumberRotation() {

	CalyxEngine::Vector3 axis  = CalyxEngine::Vector3::Up();
	float angle = 0.0f;

	if(sameNumbers_ == 1) {
		angle = std::numbers::pi_v<float> / 2.0f;
		axis  = CalyxEngine::Vector3::Right();
	} else if(sameNumbers_ == 3) {
		angle = -std::numbers::pi_v<float> / 2.0f;
	} else if(sameNumbers_ == 4) {
		angle = std::numbers::pi_v<float> / 2.0f;
	} else if(sameNumbers_ == 5) {
		angle = std::numbers::pi_v<float>;
	} else if(sameNumbers_ == 6) {
		angle = -std::numbers::pi_v<float> / 2.0f;
		axis  = CalyxEngine::Vector3::Right();
	}

	return CalyxEngine::Quaternion::MakeRotateAxisQuaternion(axis, angle);
}

void DiceSocketObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}

	BaseGameObject::SetColor({1.0f, 0.0f, 0.0f, 1.0f});

	worldTransform_.inheritScale = false;
	worldTransform_.scale.x = static_cast<float>(clearCount_ + 1u);

	sameNumbers_ = static_cast<uint32_t>(rand() % 6 + 1);
}

void DiceSocketObject::ObjectUpdate(float) {
}
