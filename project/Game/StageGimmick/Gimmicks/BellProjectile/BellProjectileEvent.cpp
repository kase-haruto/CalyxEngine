#include "BellProjectileEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(BellProjectileEvent);

BellProjectileEvent::BellProjectileEvent(const std::string& name) : StageGimmickEventBase(name) {
}

void BellProjectileEvent::OnCollisionEnter(Collider* other) {
	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// プレイヤーがイベント内に入ったら飛ばす
	if(auto target = targetObject_.lock()) {
		target->SetIsFlying(true);
	}
}

void BellProjectileEvent::EventInitialize() {
	const std::string eventPrefix = "BellProjectileEvent";
	const std::string objectName = "BellProjectileObject";
	const std::string targetName = "BellProjectileTarget";

	if(GetName().find(eventPrefix) == std::string::npos) {
		return;
	}

	// 鐘（ターゲット）
	auto target = ResolveLinkedObject<BellProjectileTarget>(bellGuid_, targetName);
	if(!target) target = FindOwnedObjectByClassName<BellProjectileTarget>(targetName);
	if(target) {
		target->SetName(targetName);
		target->SetParam(eventParam_.targetParam_);
		target->Initialize();
		bell_ = target;
		bellGuid_ = target->GetGuid();
	} else {
		auto newTarget = SceneAPI::Instantiate<BellProjectileTarget>("bell.obj", targetName);
		newTarget->SetParent(shared_from_this(), false);
		newTarget->SetParam(eventParam_.targetParam_);
		newTarget->Initialize();
		newTarget->SetTranslate({0.0f, 8.5f, 30.0f});
		bell_ = newTarget;
		bellGuid_ = newTarget->GetGuid();
	}

	// 撞木（オブジェクト）
	auto obj = ResolveLinkedObject<BellProjectileObject>(targetObjectGuid_, objectName);
	if(!obj) obj = FindOwnedObjectByClassName<BellProjectileObject>(objectName);
	if(obj) {
		obj->SetName(objectName);
		obj->SetParam(eventParam_.param_);
		obj->SetTarget(bell_.lock().get());
		obj->Initialize();
		targetObject_ = obj;
		targetObjectGuid_ = obj->GetGuid();
	} else {
		auto newObj = SceneAPI::Instantiate<BellProjectileObject>("woodenBellHammer.obj", objectName);
		newObj->SetParent(shared_from_this(), false);
		newObj->SetParam(eventParam_.param_);
		newObj->SetTarget(bell_.lock().get());
		newObj->Initialize();
		newObj->SetTranslate({0.0f, 1.0f, 20.0f});
		targetObject_	  = newObj;
		targetObjectGuid_ = newObj->GetGuid();
	}

	// 扉(左)
	const std::string doorLName = "BellProjectileDoorL";
	auto doorL = ResolveLinkedObject<BellProjectileDoor>(doorLGuid_, doorLName);
	if(!doorL) {
		for(const auto& child : GetChildren()) {
			if(child && child->GetName() == doorLName) {
				doorL = std::dynamic_pointer_cast<BellProjectileDoor>(child);
				break;
			}
		}
	}
	if(doorL) {
		doorL->SetName(doorLName);
		doorL->SetLR(0);
		doorL->SetParam(eventParam_.doorParam_);
		doorL->SetTarget(bell_.lock().get());
		doorL->Initialize();
		doorL_ = doorL;
		doorLGuid_ = doorL->GetGuid();
	} else {
		auto newDoor = SceneAPI::Instantiate<BellProjectileDoor>("sanmonDoor.obj", doorLName);
		newDoor->SetParent(shared_from_this(), false);
		newDoor->SetLR(0);
		newDoor->SetParam(eventParam_.doorParam_);
		newDoor->SetTarget(bell_.lock().get());
		newDoor->Initialize();
		newDoor->SetTranslate({-12.0f, 0.0f, 40.0f});
		doorL_	  = newDoor;
		doorLGuid_ = newDoor->GetGuid();
	}

	// 扉(右)
	const std::string doorRName = "BellProjectileDoorR";
	auto doorR = ResolveLinkedObject<BellProjectileDoor>(doorRGuid_, doorRName);
	if(!doorR) {
		for(const auto& child : GetChildren()) {
			if(child && child->GetName() == doorRName) {
				doorR = std::dynamic_pointer_cast<BellProjectileDoor>(child);
				break;
			}
		}
	}
	if(doorR) {
		doorR->SetName(doorRName);
		doorR->SetLR(1);
		doorR->SetParam(eventParam_.doorParam_);
		doorR->SetTarget(bell_.lock().get());
		doorR->Initialize();
		doorR_ = doorR;
		doorRGuid_ = doorR->GetGuid();
	} else {
		auto newDoor = SceneAPI::Instantiate<BellProjectileDoor>("sanmonDoorR.obj", doorRName);
		newDoor->SetParent(shared_from_this(), false);
		newDoor->SetLR(1);
		newDoor->SetParam(eventParam_.doorParam_);
		newDoor->SetTarget(bell_.lock().get());
		newDoor->Initialize();
		newDoor->SetTranslate({12.0f, 0.0f, 40.0f});
		doorR_	  = newDoor;
		doorRGuid_ = newDoor->GetGuid();
	}

	// 建物
	const std::string gateName = "BellProjectileGate";
	auto			  gate	   = ResolveLinkedObject<GeneralObject>(gateGuid_, gateName);
	if(!gate) gate = FindOwnedObjectByClassName<GeneralObject>(gateName);
	if(gate) {
		gate->SetName(gateName);
		gate->Initialize();
		gate_	  = gate;
		gateGuid_ = gate->GetGuid();
	} else {
		auto newGate = SceneAPI::Instantiate<GeneralObject>("sanmon.obj", gateName);
		newGate->SetParent(shared_from_this(), false);
		newGate->Initialize();
		newGate->SetScale({1.95f, 2.0f, 2.25f});
		newGate->SetTranslate({0.0f, 0.0f, 32.5f});
		gate_	  = newGate;
		gateGuid_	  = newGate->GetGuid();
	}
}

void BellProjectileEvent::EventUpdate(float) {
	if(targetObject_.expired()) {
		if(!hasSerializedEventParam_) {
			eventParam_.LoadParams();
		}
		EventInitialize();
	}
}

void BellProjectileEvent::DerivativeGui() {
	eventParam_.ShowGui();
}

void BellProjectileEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	if(derived->contains("eventParam")) {
		eventParam_.ApplyParamsFromJson(derived->at("eventParam"));
		hasSerializedEventParam_ = true;
	}
	bellGuid_ = derived->value("bellGuid", Guid{});
	targetObjectGuid_ = derived->value("targetObjectGuid", Guid{});
	doorLGuid_ = derived->value("doorLGuid", Guid{});
	doorRGuid_ = derived->value("doorRGuid", Guid{});
	
	if(auto obj = targetObject_.lock()) {
		obj->SetParam(eventParam_.param_);
	}
	if(auto target = bell_.lock()) {
		target->SetParam(eventParam_.targetParam_);
	}
	if(auto doorL = doorL_.lock()) {
		doorL->SetParam(eventParam_.doorParam_);
	}
	if(auto doorR = doorR_.lock()) {
		doorR->SetParam(eventParam_.doorParam_);
	}
}

void BellProjectileEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	eventParam_.ExtractParamsToJson(derived["eventParam"]);
	if(auto bell = bell_.lock()) {
		derived["bellGuid"] = bell->GetGuid();
	} else if(bellGuid_.isValid()) {
		derived["bellGuid"] = bellGuid_;
	}

	if(auto obj = targetObject_.lock()) {
		derived["targetObjectGuid"] = obj->GetGuid();
	} else if(targetObjectGuid_.isValid()) {
		derived["targetObjectGuid"] = targetObjectGuid_;
	}

	if(auto doorL = doorL_.lock()) {
		derived["doorLGuid"] = doorL->GetGuid();
	} else if(doorLGuid_.isValid()) {
		derived["doorLGuid"] = doorLGuid_;
	}

	if(auto doorR = doorR_.lock()) {
		derived["doorRGuid"] = doorR->GetGuid();
	} else if(doorRGuid_.isValid()) {
		derived["doorRGuid"] = doorRGuid_;
	}
}

void BellProjectileEvent::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
	RemapGuid(bellGuid_, guidMap);
	RemapGuid(targetObjectGuid_, guidMap);
	RemapGuid(doorLGuid_, guidMap);
	RemapGuid(doorRGuid_, guidMap);
}
