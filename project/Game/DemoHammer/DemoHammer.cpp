#include "DemoHammer.h"
#include "Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h"

REGISTER_SCENE_OBJECT(DemoHammer)

DemoHammer::DemoHammer() : Actor() {
}

DemoHammer::DemoHammer(const std::string& modelName, std::optional<std::string> objectName) :
Actor::Actor(modelName, objectName) {
	param_.LoadParams();
}

void DemoHammer::Initialize() {
	param_.LoadParams();
	swingAxis_	= {1.0f, 0.0f, 0.0f};
	swingAngle_ = 0.0f;
}

void DemoHammer::Update(float dt) {
	// Playerで計算された角度を適用
	float t = dt;
	t		= 1;

	worldTransform_.rotation = CalyxEngine::Quaternion::MakeRotateAxisQuaternion(swingAxis_, swingAngle_);
}

void DemoHammer::DerivativeGui() {
	param_.ShowGui();
}
