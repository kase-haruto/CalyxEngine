#include "ShojiEvent.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(ShojiEvent)

ShojiEvent::ShojiEvent(const std::string& name) : StageGimmickEventBase(name) {}

void ShojiEvent::OnCollisionEnter(Collider* other) {

	// プレイヤー以外の衝突は無視する
	if(other->GetType() != ColliderType::Type_Player) return;

	// 全ての招き猫を飛ばす
	for(auto& luckyCat : luckyCatObjs_) {
		if(luckyCat.lock()) {
			luckyCat.lock()->SetIsFlying(true);
		}
	}
}

void ShojiEvent::EventInitialize() {

	luckyCatObjs_.clear();

	const std::string eventPrefix = "ShojiEvent";
	const std::string shojiName	  = "ShojiObject";
	const std::string catName	  = "LuckyCatObject";

	if(GetName() != eventPrefix) {
		return;
	}

	// 基盤となる障子は2つ固定で管理する
	auto shojiObjects = ResolveLinkedObjects<ShojiObject>(
		std::vector<Guid>{shojiGuids_.begin(), shojiGuids_.end()},
		shojiName);

	if(shojiObjects.empty()) {
		shojiObjects = FindOwnedObjectsByClassName<ShojiObject>(shojiName);
	}

	for(size_t i = 0; i < shojiObjs_.size(); ++i) {
		std::shared_ptr<ShojiObject> shoji;
		bool isExistingShoji = false;

		if(i < shojiObjects.size()) {
			shoji = shojiObjects[i];
			isExistingShoji = true;
		} else {
			shoji = SceneAPI::Instantiate<ShojiObject>("shoji.obj", shojiName);
		}

		if(!shoji) continue;
		float posX = eventParam_.param_.shojiInterval;
		if(i == 1) { posX *= -1; }
		auto pos = shoji->GetWorldTransform().translation;
		shoji->SetTranslate({posX, pos.y, pos.z});
		shoji->SetName(shojiName);
		shoji->SetParent(shared_from_this());
		shoji->SetParam(eventParam_.param_);
		shoji->Initialize();
		shoji->CreatePaperObjects();

		shojiObjs_[i]  = shoji;
		shojiGuids_[i] = shoji->GetGuid();
	}

	// 招き猫は可変数で管理する
	auto childTargets = ResolveLinkedObjects<LuckyCatObject>(luckyCatGuids_, catName);
	if(childTargets.empty()) {
		childTargets = FindOwnedObjectsByClassName<LuckyCatObject>(catName);
	}

	if(!childTargets.empty()) {
		objectCount_		   = static_cast<uint32_t>(childTargets.size());
		eventData_.objectCount = objectCount_;
		luckyCatObjs_.resize(objectCount_);
		luckyCatGuids_.resize(objectCount_);

		for(size_t i = 0; i < childTargets.size(); ++i) {
			childTargets[i]->SetName(catName);
			childTargets[i]->SetParent(shared_from_this());
			childTargets[i]->SetParam(eventParam_.param_);
			childTargets[i]->SetShoji(shojiObjs_);
			childTargets[i]->Initialize();

			luckyCatObjs_[i]  = childTargets[i];
			luckyCatGuids_[i] = childTargets[i]->GetGuid();
		}
		return;
	}

	luckyCatObjs_.resize(objectCount_);
	luckyCatGuids_.resize(objectCount_);
	eventData_.objectCount = objectCount_;

	for(uint32_t i = 0; i < objectCount_; ++i) {
		auto luckyCat = SceneAPI::Instantiate<LuckyCatObject>("manekineko.obj", catName);
		if(!luckyCat) continue;

		luckyCat->SetParent(shared_from_this());
		luckyCat->SetParam(eventParam_.param_);
		luckyCat->SetShoji(shojiObjs_);
		luckyCat->Initialize();

		luckyCatObjs_[i]  = luckyCat;
		luckyCatGuids_[i] = luckyCat->GetGuid();
	}
}

void ShojiEvent::EventUpdate(float) {

	if(luckyCatObjs_.empty()) {
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

	if(!isOpen_) {
		int count = 0;
		for(const auto& shoji : shojiObjs_) {
			count += shoji->GetClearCount();
		}
		if(count >= eventData_.clearCount) {
			for(const auto& shoji : shojiObjs_) {
				shoji->SetIsOpen(true);
			}
			isOpen_ = true;
		}
	}
}

void ShojiEvent::DerivativeGui() {

	ImGui::Text("LuckyCatObject");
	ImGui::SameLine();
	if(ImGui::Button("+")) {
		AddLuckCatObject();
	}
	if(objectCount_ > 1) {
		ImGui::SameLine();
		if(ImGui::Button("-")) {
			DeleteLuckCatObject();
		}
	}

	eventData_.ShowGui();
	eventParam_.ShowGui();
}

void ShojiEvent::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {

	if(!derived) return;

	if(derived->contains("eventParam")) {
		eventParam_.ApplyParamsFromJson(derived->at("eventParam"));
		hasSerializedEventParam_ = true;
	}
	if(derived->contains("eventData")) {
		eventData_.ApplyParamsFromJson(derived->at("eventData"));
		objectCount_			= eventData_.objectCount;
		hasSerializedEventData_ = true;
	}

	shojiGuids_	   = derived->value("shojiGuids", std::array<Guid, 2>{});
	luckyCatGuids_ = derived->value("luckyCatGuids", std::vector<Guid>{});

	for(auto& shoji : shojiObjs_) {
		if(auto object = shoji) {
			object->SetParam(eventParam_.param_);
		}
	}
}

void ShojiEvent::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {

	eventParam_.ExtractParamsToJson(derived["eventParam"]);
	eventData_.ExtractParamsToJson(derived["eventData"]);

	std::array<Guid, 2> shojiGuids = shojiGuids_;
	for(size_t i = 0; i < shojiObjs_.size(); ++i) {
		if(auto object = shojiObjs_[i]) {
			shojiGuids[i] = object->GetGuid();
		}
	}
	derived["shojiGuids"] = shojiGuids;

	std::vector<Guid> catGuids;
	catGuids.reserve(luckyCatObjs_.size());
	for(const auto& luckyCat : luckyCatObjs_) {
		if(auto object = luckyCat.lock()) {
			catGuids.push_back(object->GetGuid());
		}
	}
	if(catGuids.empty()) catGuids = luckyCatGuids_;
	if(!catGuids.empty()) derived["luckyCatGuids"] = catGuids;
}

void ShojiEvent::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {

	for(auto& guid : shojiGuids_) {
		RemapGuid(guid, guidMap);
	}
	RemapGuids(luckyCatGuids_, guidMap);
}

void ShojiEvent::AddLuckCatObject() {

	const std::string objectName = "LuckyCatObject";
	auto luckyCat = SceneAPI::Instantiate<LuckyCatObject>("manekineko.obj", objectName);
	if(!luckyCat) return;

	luckyCat->SetParent(shared_from_this());
	luckyCat->SetParam(eventParam_.param_);
	luckyCat->Initialize();

	luckyCatObjs_.push_back(luckyCat);
	++objectCount_;
	eventData_.objectCount = objectCount_;
}

void ShojiEvent::DeleteLuckCatObject() {
	--objectCount_;
	// シーンコンテキストが存在する場合に削除処理を行う
	if(auto* ctx = SceneContext::Current()) {
		if(luckyCatObjs_[objectCount_].lock()) {
			ctx->RemoveObject(luckyCatObjs_[objectCount_].lock());
			luckyCatObjs_.resize(objectCount_);
			luckyCatGuids_.resize(objectCount_);
		}
	}
	eventData_.objectCount = objectCount_;
}
