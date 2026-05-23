#include "Shockwave.h"
#include "Engine/Objects/Collider/SphereCollider.h"
#include "Game\StageGimmick\Gimmicks\GroundSpike\GroundSpikeObject.h"
#include <algorithm>

Shockwave::Shockwave() : Actor() {
	param_.LoadParams();
}

Shockwave::Shockwave(const std::string& modelName, std::optional<std::string> objectName) : Actor::Actor(modelName, objectName) {
	param_.LoadParams();
}

void Shockwave::Initialize() {
	param_.LoadParams();

	// 球体コライダーを初期化
	InitializeCollider(ColliderKind::Sphere);
	if (collider_) {
		collider_->SetType(ColliderType::Type_PlayerAttack);
		// 敵、イベントオブジェクト、ステージギミックを対象にする
		collider_->SetTargetType(ColliderType::Type_Enemy | ColliderType::Type_EnemyAttack | ColliderType::Type_EventObject | ColliderType::Type_StageGimmick);
	}

	SetDrawEnable(false);
	isActive_ = false;
}

void Shockwave::Update(float dt) {
	if (!isActive_) return;

	timer_ += dt;
	if (timer_ >= param_.lifeTime) {
		Deactivate();
		return;
	}

	// 時間経過に合わせてスケールを拡大させる
	float progress = timer_ / param_.lifeTime;
	float easeOut = 1.0f - std::pow(1.0f - progress, 3.0f);
	float currentScale = std::lerp(param_.startScale, currentMaxScale_, easeOut);
	
	if(collider_) {
		if(auto* radius = dynamic_cast<SphereCollider*>(collider_.get())) {
			radius->SetRadius(currentScale);
		}
 		collider_->Update(worldTransform_.translation, worldTransform_.rotation);
	}
	//worldTransform_.scale = {currentScale, currentScale * 0.25f, currentScale};
	worldTransform_.scale = {currentScale, currentScale, currentScale};
	
}

void Shockwave::Activate(const CalyxEngine::Vector3& pos, float scaleMultiplier) {
	worldTransform_.translation = pos;
	worldTransform_.scale		= {param_.startScale, param_.startScale, param_.startScale};
	currentMaxScale_			= param_.endScale * scaleMultiplier;
	scaleMultiplier_			= scaleMultiplier;
	timer_						= 0.0f;
	isActive_					= true;
	isTakeDamageForStage_		= false;
	
	SetDrawEnable(true);
	if (collider_) {
		collider_->SetCollisionEnabled(true);
	}
}

void Shockwave::Deactivate() {
	isActive_ = false;
	SetDrawEnable(false);
	if (collider_) {
		collider_->SetCollisionEnabled(false);
	}
}

void Shockwave::DerivativeGui() {
	param_.ShowGui();
}

void Shockwave::OnCollisionEnter(Collider* other) {
	if (!isActive_) return;

	// ギミックなどへの干渉 or 相手側に追加する
	BaseGameObject* otherObj = other->GetOwner();
	if(!otherObj) return;

	if(auto* spike = dynamic_cast<GroundSpikeObject*>(otherObj)) {
		isTakeDamageForStage_ = true;
	}

}

bool Shockwave::IsTakeDamageForStage() {
	if(!isTakeDamageForStage_) return false;
	bool result = isTakeDamageForStage_;
	isTakeDamageForStage_ = !isTakeDamageForStage_;
	return result;
}
