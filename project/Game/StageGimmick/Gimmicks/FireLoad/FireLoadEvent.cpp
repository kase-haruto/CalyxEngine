#include "FireLoadEvent.h"

#include <cmath>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Utility/SceneUtility.h>

REGISTER_SCENE_OBJECT(FireLoadEvent)

FireLoadEvent::FireLoadEvent(const std::string& name) : StageGimmickEventBase(name) {}

void FireLoadEvent::OnCollisionEnter(Collider* other) {

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// プレイヤーがイベント内に入ったら燃える
	for(auto& target : targetObjects_) {
		if(auto lockedTarget = target.lock()) {
			lockedTarget->BurnActive();
		}
	}
}

void FireLoadEvent::OnCollisionExit(Collider* other) {

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;


}

void FireLoadEvent::SetTarget(const std::shared_ptr<FireLoadObject>& target) {
	targetObjects_.push_back(target);
	if(target) targetObjectGuids_.push_back(target->GetGuid());
}

void FireLoadEvent::EventInitialize() {

	if(!hasSerializedEventParam_) {
		eventParam_.LoadParams();
	}

	worldTransform_.scale.x = 20.0f;
	worldTransform_.scale.y = 6.0f;

	// 列の数(奥行き)と幅に基づいてオブジェクト数を計算する
	const int width = (std::max)(1, eventParam_.param_.width);
	const int depth = (std::max)(1, eventParam_.param_.depth);
	objectCount_	= width * depth;
	const std::string eventPrefix = "FireLoadEvent";
	const std::string objectName	 = "FireLoadObject";

	if(GetName() != eventPrefix) {
		return;
	}

	auto childTargets = ResolveLinkedObjects<FireLoadObject>(targetObjectGuids_, objectName);
	if(childTargets.empty()) childTargets = FindOwnedObjectsByClassName<FireLoadObject>(objectName);
	if(!childTargets.empty()) {
		objectCount_ = static_cast<uint32_t>(childTargets.size());
	}
	targetObjects_.resize(objectCount_);
	targetObjectGuids_.resize(objectCount_);

	// グリッドの間隔
	const float xInterval = 2.5f;
	const float zInterval = 2.0f;

	// シーンから対応するオブジェクトを生成する
	for(uint32_t i = 0; i < objectCount_; ++i) {
		std::shared_ptr<FireLoadObject> targetObject =
			i < childTargets.size() ? childTargets[i] : nullptr;
		if(!targetObject) {
			targetObject = SceneAPI::Instantiate<FireLoadObject>("cone.obj", objectName);
		}
		if(targetObject) {
			targetObject->SetName(objectName);
			targetObject->SetParent(shared_from_this());
			targetObject->Initialize();
			targetObject->SetParam(eventParam_.param_);
			targetObject->GetWorldTransform().inheritScale = false;

			// 長方形(グリッド)状に配置する
			int row = i / width; // 行 (奥方向)
			int col = i % width; // 列 (左右方向)

			CalyxEngine::Vector3 pos = {0.0f, 0.0f, 0.0f};

			// 中央を原点(0,0,0)とするように配置を計算
			// 横幅(X軸)の中央揃え
			pos.x = (static_cast<float>(col) - static_cast<float>(width - 1) * 0.5f) * xInterval;
			// 奥行き(Z軸)
			pos.z = static_cast<float>(row) * zInterval + eventParam_.defaultZSpace;

			// 少しバラけさせる (ノイズ)
			pos.x += std::sin(static_cast<float>(i) * 1.5f) * 0.3f;
			pos.z += std::cos(static_cast<float>(i) * 1.5f) * 0.3f;

			targetObject->GetWorldTransform().translation = pos;

			targetObjects_[i] = (targetObject);
			targetObjectGuids_[i] = targetObject->GetGuid();
		}
	}
}

void FireLoadEvent::EventUpdate(float dt) {

	if(targetObjects_.empty()) {
		EventInitialize();
	} else {

		for(const auto& target : targetObjects_) {
			if(auto obj = target.lock()) {
				CalyxEngine::Vector3 targetPos = obj->GetWorldTransform().translation;
				targetPos.y					   = 0.01f;
				targetPos.y += -(worldTransform_.translation.y) + 1.0f;
				obj->GetWorldTransform().translation = targetPos;
			}
		}
	}

	dt;
}

void FireLoadEvent::DerivativeGui() {
	eventParam_.ShowGui();
}

void FireLoadEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	if(derived->contains("eventParam")) {
		eventParam_.ApplyParamsFromJson(derived->at("eventParam"));
		hasSerializedEventParam_ = true;
	}
	targetObjectGuids_ = derived->value("targetObjectGuids", std::vector<Guid>{});
	for(auto& target : targetObjects_) {
		if(auto object = target.lock()) {
			object->SetParam(eventParam_.param_);
		}
	}
}

void FireLoadEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	eventParam_.ExtractParamsToJson(derived["eventParam"]);
	std::vector<Guid> guids;
	guids.reserve(targetObjects_.size());
	for(const auto& target : targetObjects_) {
		if(auto object = target.lock()) {
			guids.push_back(object->GetGuid());
		}
	}
	if(guids.empty()) guids = targetObjectGuids_;
	if(!guids.empty()) derived["targetObjectGuids"] = guids;
}

void FireLoadEvent::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
	RemapGuids(targetObjectGuids_, guidMap);
}
