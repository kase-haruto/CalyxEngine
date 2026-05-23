#include "BreakableWallEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(BreakableWallEvent)

BreakableWallEvent::BreakableWallEvent(const std::string& name) : StageGimmickEventBase(name) {}

void BreakableWallEvent::SetTarget(const std::shared_ptr<BreakableWallObject>& target) {
	targetObject_	  = target;
	targetObjectGuid_ = target ? target->GetGuid() : Guid{};
}

void BreakableWallEvent::OnCollisionEnter(Collider* other) {

	// ハンマー判定かどうか確認する
	// ギミックなどへの干渉 or 相手側に追加する
	BaseGameObject* otherObj = other->GetOwner();
	if(otherObj && other->GetType() != ColliderType::Type_PlayerAttack) {
		return;
	}

	// プレイヤーの攻撃に当たったら床を壊す
	auto floor = targetObject_.lock();
	if(!floor) {
		return;
	}
	floor->Break();

	// イベントを無効化する

	isActive_ = false;
}

void BreakableWallEvent::EventInitialize() {

	param_.LoadParams();

	if(!targetObject_.expired()) {
		return;
	}

	const std::string eventPrefix  = "BreakableWallEvent";
	const std::string objectPrefix = "BreakableWallObject";

	auto object = ResolveLinkedObject<BreakableWallObject>(targetObjectGuid_, objectPrefix);
	if(!object) object = FindOwnedObjectByClassName<BreakableWallObject>(objectPrefix);
	if(object) {
		targetObject_ = object;
		object->SetName(objectPrefix);
		SetTarget(object);
		return;
	}
	// シーンから対応するオブジェクトが無ければ生成する
	targetObject_ = SceneAPI::Instantiate<BreakableWallObject>("debugCube.obj", objectPrefix);
	targetObject_.lock()->SetParent(shared_from_this());
	targetObjectGuid_ = targetObject_.lock()->GetGuid();
	targetObject_.lock()->Initialize();
	targetObject_.lock()->GetWorldTransform().translation.y -= 0.5f;
	targetObject_.lock()->GetWorldTransform().inheritScale = false;
}

void BreakableWallEvent::EventUpdate(float) {

	if(!targetObject_.lock()) {
		EventInitialize();
	}
}

void BreakableWallEvent::DerivativeGui() {
	param_.ShowGui();
}

void BreakableWallEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	targetObjectGuid_ = derived->value("targetObjectGuid", Guid{});
}

void BreakableWallEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	if(auto target = targetObject_.lock()) {
		derived["targetObjectGuid"] = target->GetGuid();
	} else if(targetObjectGuid_.isValid()) {
		derived["targetObjectGuid"] = targetObjectGuid_;
	}
}

void BreakableWallEvent::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
	RemapGuid(targetObjectGuid_, guidMap);
}
