#include "GroundSpikeEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(GroundSpikeEvent)

GroundSpikeEvent::GroundSpikeEvent(const std::string& name) : StageGimmickEventBase(name) {}

void GroundSpikeEvent::SetTarget(const std::shared_ptr<GroundSpikeObject>& target) {
	targetObject_ = target;
	targetObjectGuid_ = target ? target->GetGuid() : Guid{};
}

void GroundSpikeEvent::OnCollisionEnter(Collider* other) {

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// プレイヤーがイベント内に入ったら歯を出す
	if(auto target = targetObject_.lock()) {
		target->Spike();
	}
}

void GroundSpikeEvent::EventInitialize() {

	if(!targetObject_.expired()) {
		return;
	}

	const std::string eventPrefix  = "GroundSpikeEvent";
	const std::string objectPrefix = "GroundSpikeObject";
	const std::string predictionCirclePrefix = "PredictionCircle";

	if(GetName() != eventPrefix) {
		return;
	}

	auto object = ResolveLinkedObject<GroundSpikeObject>(targetObjectGuid_, objectPrefix);
	if(!object) object = FindOwnedObjectByClassName<GroundSpikeObject>(objectPrefix);
	if(object) {
		targetObject_ = object;
		object->SetName(objectPrefix);
		SetTarget(object);
		object->SetParam(eventParam_.param_);
	} else {
		// シーンから対応するオブジェクトが無ければ生成する
		targetObject_ = SceneAPI::Instantiate<GroundSpikeObject>("backTeeth.obj", objectPrefix);
		targetObject_.lock()->SetParent(shared_from_this());
		targetObjectGuid_ = targetObject_.lock()->GetGuid();
		targetObject_.lock()->SetParam(eventParam_.param_);
		targetObject_.lock()->Initialize();
	}

	auto prediction = ResolveLinkedObject<PredictionCircle>(targetObjectGuid_, predictionCirclePrefix);
	if(!prediction) prediction = FindOwnedObjectByClassName<PredictionCircle>(predictionCirclePrefix);
	if(prediction) {
		predictionObject_ = prediction;
		prediction->SetName(predictionCirclePrefix);
		prediction->SetObjectScale(1.2f);
		prediction->SetParent(targetObject_.lock());
	} else {
		// シーンから対応するオブジェクトが無ければ生成する
		predictionObject_ = SceneAPI::Instantiate<PredictionCircle>("PredictionCircle.obj", predictionCirclePrefix);
		predictionObject_.lock()->SetParent(targetObject_.lock());
		predictionObject_.lock()->SetObjectScale(1.2f);
		targetObjectGuid_ = predictionObject_.lock()->GetGuid();
		predictionObject_.lock()->Initialize();
		predictionObject_.lock()->SetTexture("circle/groundPrediction.png");
	}
}

void GroundSpikeEvent::EventUpdate(float) {

	if(!targetObject_.lock()) {
		if(!hasSerializedEventParam_) {
			eventParam_.LoadParams();
		}
		EventInitialize();
	}

	if(targetObject_.lock() && predictionObject_.lock()) {
		CalyxEngine::Vector3 targetPos = targetObject_.lock()->GetWorldPosition();
		targetPos.y = 0.1f;
		predictionObject_.lock()->SetTranslate(targetPos);
	}
}

void GroundSpikeEvent::DerivativeGui() {

	eventParam_.ShowGui();
}

void GroundSpikeEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	if(derived->contains("eventParam")) {
		eventParam_.ApplyParamsFromJson(derived->at("eventParam"));
		hasSerializedEventParam_ = true;
	}
	targetObjectGuid_ = derived->value("targetObjectGuid", Guid{});
	if(auto target = targetObject_.lock()) {
		target->SetParam(eventParam_.param_);
	}
	predictionObjectGuid_ = derived->value("predictionObjectGuid", Guid{});
}

void GroundSpikeEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	eventParam_.ExtractParamsToJson(derived["eventParam"]);
	if(auto target = targetObject_.lock()) {
		derived["targetObjectGuid"] = target->GetGuid();
	} else if(targetObjectGuid_.isValid()) {
		derived["targetObjectGuid"] = targetObjectGuid_;
	}

	if(auto target = predictionObject_.lock()) {
		derived["predictionObjectGuid"] = target->GetGuid();
	} else if(predictionObjectGuid_.isValid()) {
		derived["predictionObjectGuid"] = predictionObjectGuid_;
	}
}

void GroundSpikeEvent::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
	RemapGuid(targetObjectGuid_, guidMap);
	RemapGuid(predictionObjectGuid_, guidMap);
}
