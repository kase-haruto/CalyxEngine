#include "ProjectileFireEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(ProjectileFireEvent)

ProjectileFireEvent::ProjectileFireEvent(const std::string& name) : StageGimmickEventBase(name) {}

void ProjectileFireEvent::SetTarget(const std::shared_ptr<ProjectileObject>& target) {
	targetObject_ = target;
	targetObjectGuid_ = target ? target->GetGuid() : Guid{};
}

void ProjectileFireEvent::OnCollisionEnter(Collider* other) {

	other;

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// プレイヤーがイベント内に入ったら飛んでくる
	if(auto target = targetObject_.lock()) {
		target->SetIsFlying(true);
	}
}

void ProjectileFireEvent::EventInitialize() {

	param_.LoadParams();

	if(!targetObject_.expired()) {
		return;
	}

	const std::string eventPrefix  = "ProjectileFireEvent";
	const std::string objectPrefix = "ProjectileObject";
	if(GetName() != eventPrefix) {
		return;
	}

	auto object = ResolveLinkedObject<ProjectileObject>(targetObjectGuid_, objectPrefix);
	if(!object) object = FindOwnedObjectByClassName<ProjectileObject>(objectPrefix);
	if(object) {
		targetObject_ = object;
		object->SetParam(param_.param);
		object->SetParent(shared_from_this());
		object->SetName(objectPrefix);
		SetTarget(object);
		return;
	}
	// シーンから対応するオブジェクトが無ければ生成する
	targetObject_ = SceneAPI::Instantiate<ProjectileObject>("crane.obj", objectPrefix);
	targetObject_.lock()->SetParent(shared_from_this());
	targetObject_.lock()->SetParam(param_.param);
	targetObjectGuid_ = targetObject_.lock()->GetGuid();
	targetObject_.lock()->Initialize();
	targetObject_.lock()->GetWorldTransform().translation.y -= 0.5f;
	targetObject_.lock()->GetWorldTransform().inheritScale = false;
}

void ProjectileFireEvent::EventUpdate(float dt) {

	if(!targetObject_.lock()) {
		EventInitialize();
	}

	dt;
}

void ProjectileFireEvent::DerivativeGui() {
	param_.ShowGui();
}

void ProjectileFireEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	targetObjectGuid_ = derived->value("targetObjectGuid", Guid{});
}

void ProjectileFireEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	if(auto target = targetObject_.lock()) {
		derived["targetObjectGuid"] = target->GetGuid();
	} else if(targetObjectGuid_.isValid()) {
		derived["targetObjectGuid"] = targetObjectGuid_;
	}
}

void ProjectileFireEvent::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
	RemapGuid(targetObjectGuid_, guidMap);
}
