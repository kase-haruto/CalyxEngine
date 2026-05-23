#include "DroolRainEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

#include <algorithm>

REGISTER_SCENE_OBJECT(DroolRainEvent)

DroolRainEvent::DroolRainEvent(const std::string& name) : StageGimmickEventBase(name) {}

namespace {

	void ApplyJson(const nlohmann::json& j, DroolRainParam& param) {
		param.objectScale_ = j.value("objectScale", param.objectScale_);
		param.velocityY_ = j.value("velocityY", param.velocityY_);
		param.accelerationY_ = j.value("accelerationY", param.accelerationY_);
		param.airScaleSpeed_ = j.value("airScaleSpeed", param.airScaleSpeed_);
		param.groundLifeTime_ = j.value("groundLifeTime", param.groundLifeTime_);
		if(j.contains("groundScaleSpeed")) {
			param.groundScaleSpeed_ = j.at("groundScaleSpeed").get<CalyxEngine::Vector3>();
		}
		param.groundVelocityY_ = j.value("groundVelocityY", param.groundVelocityY_);
	}

	template <class TObject>
	std::shared_ptr<TObject> FindDirectChildOfType(const SceneObject& parent) {
		for(const auto& child : parent.GetChildren()) {
			if(auto casted = std::dynamic_pointer_cast<TObject>(child)) {
				return casted;
			}
		}
		return nullptr;
	}

	template <class TObject>
	std::vector<std::shared_ptr<TObject>> FindDirectChildrenOfType(const SceneObject& parent) {
		std::vector<std::shared_ptr<TObject>> result;
		for(const auto& child : parent.GetChildren()) {
			if(auto casted = std::dynamic_pointer_cast<TObject>(child)) {
				result.push_back(std::move(casted));
			}
		}

		std::sort(result.begin(), result.end(),
				  [](const std::shared_ptr<TObject>& lhs,
					 const std::shared_ptr<TObject>& rhs) {
					  if(lhs->GetName() != rhs->GetName()) return lhs->GetName() < rhs->GetName();
					  return lhs->GetGuid().ToString() < rhs->GetGuid().ToString();
				  });
		return result;
	}
} // namespace

void DroolRainEvent::OnCollisionEnter(Collider* other) {

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// プレイヤーがイベント内に入ったら降らせる
	for(auto& target : targetObjects_) {
		if(auto lockedTarget = target.lock()) {
			lockedTarget->SetIsRaining(true);
		}
	}
}

void DroolRainEvent::OnCollisionExit(Collider* other) {

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// プレイヤーがイベント内から出たら降らせるのをやめる
	for(auto& target : targetObjects_) {
		if(auto lockedTarget = target.lock()) {
			//lockedTarget->SetIsRaining(false);
		}
	}
}

void DroolRainEvent::SetTarget(const std::shared_ptr<DroolRainObject>& target) {
	targetObjects_.push_back(target);
	if(target) targetObjectGuids_.push_back(target->GetGuid());
}

void DroolRainEvent::EventInitialize() {

	targetObjects_.clear();
	predictionCircles_.clear();
	const std::string eventPrefix  = "DroolRainEvent";
	const std::string objectName = "DroolRainObject";
	const std::string predictionCircleName = "PredictionCircle";

	if(GetName() != eventPrefix) {
		return;
	}

	auto childTargets = ResolveLinkedObjects<DroolRainObject>(targetObjectGuids_, objectName);
	if(childTargets.empty()) childTargets = FindOwnedObjectsByClassName<DroolRainObject>(objectName);
	if(!childTargets.empty()) {
		objectCount_ = static_cast<uint32_t>(childTargets.size());
	}

	targetObjects_.resize(objectCount_);
	predictionCircles_.resize(objectCount_);
	targetObjectGuids_.resize(objectCount_);
	predictionCircleGuids_.resize(objectCount_);
	eventData_.objectCount = objectCount_;

	for(uint32_t i = 0; i < objectCount_; ++i) {
		std::shared_ptr<DroolRainObject> targetObject =
			i < childTargets.size() ? childTargets[i] : nullptr;
		if(!targetObject) {
			targetObject = SceneAPI::Instantiate<DroolRainObject>("waterDrop.obj", objectName);
		}
		if(targetObject) {
			targetObject->SetName(objectName);
			targetObject->SetParent(shared_from_this());
			targetObject->SetParam(eventParam_.param_);
			targetObject->Initialize();
			targetObjects_[i] = (targetObject);
			targetObjectGuids_[i] = targetObject->GetGuid();
		}
	}

	for(uint32_t i = 0; i < objectCount_; ++i) {
		auto targetObject = ResolveLinkedObject<PredictionCircle>(predictionCircleGuids_[i], predictionCircleName);
		if(!targetObject) {
			auto target = targetObjects_[i].lock();
			auto childPredictionCircles = target
				? FindChildrenByClassName<PredictionCircle>(*target, predictionCircleName)
				: std::vector<std::shared_ptr<PredictionCircle>>{};
			targetObject = childPredictionCircles.empty() ? nullptr : childPredictionCircles.front();
		}
		if(!targetObject) {
			targetObject = SceneAPI::Instantiate<PredictionCircle>("PredictionCircle.obj", predictionCircleName);
		}
		if(targetObject) {
			targetObject->SetName(predictionCircleName);
			targetObject->SetParent(targetObjects_[i].lock());
			targetObject->Initialize();
			predictionCircles_[i] = (targetObject);
			predictionCircleGuids_[i] = targetObject->GetGuid();
		}
	}
}

void DroolRainEvent::EventUpdate(float) {

	if(targetObjects_.empty()) {
		if(!hasSerializedEventParam_) {
			eventParam_.LoadParams();
		}
		if(!hasSerializedObjectCount_) {
			eventData_.SetEventName(GetName());
			eventData_.LoadParams();
			objectCount_ = eventData_.objectCount;
		}
		EventInitialize();
	} else {

		uint32_t index = 0;
		for(const auto& target : targetObjects_) {
			if(target.lock() && predictionCircles_[index].lock()) {
				CalyxEngine::Vector3 targetPos = target.lock()->GetWorldPosition();
				targetPos.y					   = 0.01f;
				predictionCircles_[index].lock()->SetTranslate(targetPos);
			}
			index++;
		}
	}
}

void DroolRainEvent::DerivativeGui() {

	ImGui::Text("DroolRainObject");
	ImGui::SameLine();
	if(ImGui::Button("+")) {
		AddDroolObject();
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

void DroolRainEvent::AddDroolObject() {

	const std::string objectName			 = "DroolRainObject";
	const std::string eventPrefix			 = "DroolRainEvent";
	const std::string predictionCirclePrefix = "PredictionCircle";

	std::string predictionCircleName = predictionCirclePrefix;
	if(GetName().find(eventPrefix) == 0) {
		predictionCircleName += GetName().substr(eventPrefix.size());
	}

	auto targetObject = SceneAPI::Instantiate<DroolRainObject>("waterDrop.obj", objectName);
	if(!targetObject) return;

	targetObject->SetParent(shared_from_this());
	targetObject->SetParam(eventParam_.param_);
	targetObject->Initialize();

	auto predictionCircle = SceneAPI::Instantiate<PredictionCircle>("PredictionCircle.obj", predictionCircleName);
	if(predictionCircle) {
		predictionCircle->SetParent(targetObject);
		predictionCircle->Initialize();
	}

	targetObjects_.push_back(targetObject);
	predictionCircles_.push_back(predictionCircle);
	++objectCount_;
	eventData_.objectCount = objectCount_;
}

void DroolRainEvent::DeleteDroolObject() {

	--objectCount_;
	// シーンコンテキストが存在する場合に削除処理を行う
	if(auto* ctx = SceneContext::Current()) {
		if(targetObjects_[objectCount_].lock()) {
			ctx->RemoveObject(targetObjects_[objectCount_].lock());
			ctx->RemoveObject(predictionCircles_[objectCount_].lock());
			targetObjects_.resize(objectCount_);
			predictionCircles_.resize(objectCount_);
			targetObjectGuids_.resize(objectCount_);
			predictionCircleGuids_.resize(objectCount_);
		}
	}
	eventData_.objectCount = objectCount_;
}

void DroolRainEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	if(derived->contains("eventParam")) {
		const auto& eventParamJson = derived->at("eventParam");
		if(eventParamJson.contains("fields")) {
			eventParam_.ApplyParamsFromJson(eventParamJson);
		} else {
			ApplyJson(eventParamJson, eventParam_.param_);
		}
		hasSerializedEventParam_ = true;
	}
	targetObjectGuids_ = derived->value("targetObjectGuids", std::vector<Guid>{});
	predictionCircleGuids_ = derived->value("predictionCircleGuids", std::vector<Guid>{});

	if(derived->contains("objectCount")) {
		objectCount_ = derived->value("objectCount", objectCount_);
		hasSerializedObjectCount_ = true;
	} else if(!targetObjectGuids_.empty()) {
		objectCount_ = static_cast<uint32_t>(targetObjectGuids_.size());
		hasSerializedObjectCount_ = true;
	}
	eventData_.objectCount = objectCount_;
	for(auto& target : targetObjects_) {
		if(auto object = target.lock()) {
			object->SetParam(eventParam_.param_);
		}
	}
}

void DroolRainEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	eventParam_.ExtractParamsToJson(derived["eventParam"]);
	derived["objectCount"] = objectCount_;

	std::vector<Guid> targetGuids;
	targetGuids.reserve(targetObjects_.size());
	for(const auto& target : targetObjects_) {
		if(auto object = target.lock()) targetGuids.push_back(object->GetGuid());
	}
	if(targetGuids.empty()) targetGuids = targetObjectGuids_;
	if(!targetGuids.empty()) derived["targetObjectGuids"] = targetGuids;

	std::vector<Guid> predictionGuids;
	predictionGuids.reserve(predictionCircles_.size());
	for(const auto& prediction : predictionCircles_) {
		if(auto object = prediction.lock()) predictionGuids.push_back(object->GetGuid());
	}
	if(predictionGuids.empty()) predictionGuids = predictionCircleGuids_;
	if(!predictionGuids.empty()) derived["predictionCircleGuids"] = predictionGuids;
}

void DroolRainEvent::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
	RemapGuids(targetObjectGuids_, guidMap);
	RemapGuids(predictionCircleGuids_, guidMap);
}
