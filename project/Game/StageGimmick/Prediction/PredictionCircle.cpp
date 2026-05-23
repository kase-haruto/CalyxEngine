#include "PredictionCircle.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

#include "Game\StageGimmick\Gimmicks\DroolRain\DroolRainObject.h"

REGISTER_SCENE_OBJECT(PredictionCircle)

PredictionCircle::PredictionCircle(
	const std::string& modelName, 
	std::optional<std::string> objectName): 
	BaseGameObject(modelName, objectName) {}

void PredictionCircle::Initialize() {

	BaseGameObject::Initialize();

	worldTransform_.translation.y = 0.01f;
	worldTransform_.inheritScale = false;
	worldTransform_.inheritTranslate = false;

	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}

	SetColor({1.0f, 0.0f, 0.0f, 1.0f});
}

void PredictionCircle::Update(float dt) {

	if(targetObjectY_ == 0.0f && worldTransform_.parent) {
		targetObjectY_ = worldTransform_.parent->GetWorldPosition().y;
	}

	// 高さによってスケールを変化させる
	const auto& parent = worldTransform_.parent;
	if(parent) {
		float scale = parent->GetWorldPosition().y / targetObjectY_;
		worldTransform_.scale = CalyxEngine::Vector3::One() * scale * objectScale_;

		radian_ += dt;
		worldTransform_.rotation = CalyxEngine::Quaternion::MakeRotateY(radian_);
	}
}
