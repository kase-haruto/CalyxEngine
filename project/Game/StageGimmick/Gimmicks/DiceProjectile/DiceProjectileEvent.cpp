#include "DiceProjectileEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(DiceProjectileEvent);

DiceProjectileEvent::DiceProjectileEvent(const std::string& name) : StageGimmickEventBase(name) {}

void DiceProjectileEvent::OnCollisionEnter(Collider* other) {

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// 全てのサイコロを飛ばす
	for(auto& target : targetObjects_) {
		if(target.lock()) {
			target.lock()->SetIsFlying(true);
			target.lock()->SetIsFlying(true);
		}
	}
}

void DiceProjectileEvent::EventInitialize() {

	targetObjects_.clear();

	const std::string eventPrefix  = "DiceProjectileEvent";
	const std::string objectName = "DiceProjectileObject";
	const std::string socketPrefix = "DiceSocketObject";

	if(GetName() != eventPrefix) {
		return;
	}

	auto object = ResolveLinkedObject<DiceSocketObject>(socketGuid_, socketPrefix);
	if(!object) object = FindOwnedObjectByClassName<DiceSocketObject>(socketPrefix);
	if(object) {
		object->SetName(socketPrefix);
		object->SetClearCount(eventData_.clearCount);
		object->Initialize();
		socket_ = object;
		socketGuid_ = object->GetGuid();

	// シーンから対応するオブジェクトが無ければ生成する
	} else {
		socket_ = SceneAPI::Instantiate<DiceSocketObject>("debugCube.obj", socketPrefix);
		socket_.lock()->SetParent(shared_from_this());
		socket_.lock()->SetClearCount(eventData_.clearCount);
		socket_.lock()->Initialize();
		socketGuid_ = socket_.lock()->GetGuid();
	}

	auto childTargets = ResolveLinkedObjects<DiceProjectileObject>(targetObjectGuids_, objectName);
	if(childTargets.empty()) childTargets = FindOwnedObjectsByClassName<DiceProjectileObject>(objectName);
	if(!childTargets.empty()) {
		objectCount_ = static_cast<uint32_t>(childTargets.size());
		eventData_.objectCount = objectCount_;
		targetObjects_.resize(objectCount_);
		targetObjectGuids_.resize(objectCount_);

		for(size_t i = 0; i < childTargets.size(); ++i) {
			targetObjects_[i] = childTargets[i];
			targetObjectGuids_[i] = childTargets[i]->GetGuid();
			childTargets[i]->SetName(objectName);
			childTargets[i]->SetParam(eventParam_.param_);
			childTargets[i]->SetSocket(socket_.lock().get());
			childTargets[i]->Initialize();
		}
		return;
	}

	targetObjects_.resize(objectCount_);
	targetObjectGuids_.resize(objectCount_);
	eventData_.objectCount = objectCount_;

	// シーンから対応するオブジェクトを生成する
	for(uint32_t i = 0; i < objectCount_; ++i) {
		auto targetObject = SceneAPI::Instantiate<DiceProjectileObject>("dice.obj", objectName);
		if(targetObject) {
			targetObject->SetParent(shared_from_this());
			targetObject->SetParam(eventParam_.param_);
			targetObject->SetSocket(socket_.lock().get());
			targetObject->Initialize();
			targetObjects_[i] = (targetObject);
			targetObjectGuids_[i] = targetObject->GetGuid();
		}
	}
}

void DiceProjectileEvent::EventUpdate(float) {

	if(targetObjects_.empty()) {
		if(!hasSerializedEventParam_) {
			eventParam_.LoadParams();
		}
		if(!hasSerializedEventData_) {
			eventData_.SetEventName(GetName());
			eventData_.LoadParams();
		}
		objectCount_ = eventData_.objectCount;
		EventInitialize();
	}

}

void DiceProjectileEvent::DerivativeGui() {

	ImGui::Text("DroolRainObject");
	ImGui::SameLine();
	if(ImGui::Button("+")) {
		AddDiceProjectileObject();
	}
	if(objectCount_ > 1) {
		ImGui::SameLine();
		if(ImGui::Button("-")) {
			DeleteDroolObject();
		}
	}
	eventData_.ShowGui();
	eventParam_.ShowGui();
}

void DiceProjectileEvent::AddDiceProjectileObject() {

	const std::string objectName   = "DiceProjectileObject";
	auto targetObject = SceneAPI::Instantiate<DiceProjectileObject>("dice.obj", objectName);
	if(!targetObject) return;

	targetObject->SetParent(shared_from_this());
	targetObject->SetParam(eventParam_.param_);
	targetObject->SetSocket(socket_.lock().get());
	targetObject->Initialize();

	targetObjects_.push_back(targetObject);
	++objectCount_;
	eventData_.objectCount = objectCount_;
}

void DiceProjectileEvent::DeleteDroolObject() {

	--objectCount_;
	// シーンコンテキストが存在する場合に削除処理を行う
	if(auto* ctx = SceneContext::Current()) {
		if(targetObjects_[objectCount_].lock()) {
			ctx->RemoveObject(targetObjects_[objectCount_].lock());
			targetObjects_.resize(objectCount_);
			targetObjectGuids_.resize(objectCount_);
		}
	}
	eventData_.objectCount = objectCount_;
}

void DiceProjectileEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	if(derived->contains("eventParam")) {
		eventParam_.ApplyParamsFromJson(derived->at("eventParam"));
		hasSerializedEventParam_ = true;
	}
	if(derived->contains("eventData")) {
		eventData_.ApplyParamsFromJson(derived->at("eventData"));
		objectCount_ = eventData_.objectCount;
		hasSerializedEventData_ = true;
	}
	socketGuid_ = derived->value("socketGuid", Guid{});
	targetObjectGuids_ = derived->value("targetObjectGuids", std::vector<Guid>{});
	for(auto& target : targetObjects_) {
		if(auto object = target.lock()) {
			object->SetParam(eventParam_.param_);
		}
	}
}

void DiceProjectileEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	eventParam_.ExtractParamsToJson(derived["eventParam"]);
	eventData_.ExtractParamsToJson(derived["eventData"]);
	if(auto socket = socket_.lock()) {
		derived["socketGuid"] = socket->GetGuid();
	} else if(socketGuid_.isValid()) {
		derived["socketGuid"] = socketGuid_;
	}

	std::vector<Guid> guids;
	guids.reserve(targetObjects_.size());
	for(const auto& target : targetObjects_) {
		if(auto object = target.lock()) guids.push_back(object->GetGuid());
	}
	if(guids.empty()) guids = targetObjectGuids_;
	if(!guids.empty()) derived["targetObjectGuids"] = guids;
}

void DiceProjectileEvent::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
	RemapGuid(socketGuid_, guidMap);
	RemapGuids(targetObjectGuids_, guidMap);
}