#include "BellProjectileTarget.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Math/MathUtil.h>

REGISTER_SCENE_OBJECT(BellProjectileTarget)

BellProjectileTarget::BellProjectileTarget(const std::string& modelName, std::optional<std::string> objectName) 
	: StageGimmickObjectBase(modelName, objectName) {
}

const CalyxEngine::Vector3 BellProjectileTarget::GetTargetPos() const {
	return worldTransform_.translation;
}

void BellProjectileTarget::SetIsRing(bool ring) {
	if(!isRing_ && ring) {
		// 鳴った瞬間のエフェクト
		wobbleT_ = 0.0f;
	}
	isRing_ = ring;
}

void BellProjectileTarget::ObjectInitialize() {
	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_StageGimmick);
		collider_->SetTargetType(ColliderType::Type_None);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
	worldTransform_.scale = param_.scale;
	isRing_ = false;
	wobbleT_ = 0.0f;
}

void BellProjectileTarget::ObjectUpdate(float dt) {
	ChangeScale();

	if (isRing_) {
		wobbleT_ += dt;

		if(worldTransform_.translation.y <= 30.0f) {
			worldTransform_.translation.y += param_.moveSpeed * dt;
		}

		// 鐘を鳴らす揺れ（減衰するサイン波）
		float frequency = 15.0f;
		float intensity = 0.2f * std::exp(-wobbleT_ * 2.0f);
		float angle	  = std::sin(wobbleT_ * frequency) * intensity;

		worldTransform_.rotation = CalyxEngine::Quaternion::MakeRotateZ(angle);
	}
}

