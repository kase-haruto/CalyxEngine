#include "StageGimmickManager.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <externals/imgui/imgui.h>

#include "Game/StageGimmick/Gimmicks/BreakableFloor/BreakableFloorEvent.h"
#include "Game/StageGimmick/Gimmicks/BreakableFloor/BreakableFloorObject.h"
#include "Game/StageGimmick/Gimmicks/GroundSpike/GroundSpikeEvent.h"
#include "Game/StageGimmick/Gimmicks/GroundSpike/GroundSpikeObject.h"
#include "Game/StageGimmick/Gimmicks/DroolRain/DroolRainEvent.h"
#include "Game/StageGimmick/Gimmicks/DroolRain/DroolRainObject.h"
#include "Game\StageGimmick\Gimmicks\Projectile\ProjectileFireEvent.h"
#include "Game\StageGimmick\Gimmicks\Projectile\ProjectileObject.h"

namespace {
	template <class TObject>
	std::vector<std::shared_ptr<TObject>> FindChildrenByClassName(
		const SceneObject& parent,
		std::string_view className) {
		std::vector<std::shared_ptr<TObject>> result;
		for(const auto& child : parent.GetChildren()) {
			if(child && child->GetObjectClassName() == className) {
				result.push_back(std::static_pointer_cast<TObject>(child));
			}
		}
		return result;
	}

	template <class TObject>
	std::vector<std::shared_ptr<TObject>> FindObjectsByClassName(SceneObjectLibrary& lib,
																std::string_view className) {
		std::vector<std::shared_ptr<TObject>> result;
		for(auto& object : lib.FindByClassName(className)) {
			result.push_back(std::static_pointer_cast<TObject>(object));
		}
		return result;
	}

	template <class TObject>
	std::vector<std::shared_ptr<StageGimmickObjectBase>> ToGimmickObjects(
		const std::vector<std::shared_ptr<TObject>>& objects) {
		std::vector<std::shared_ptr<StageGimmickObjectBase>> result;
		result.reserve(objects.size());
		for(const auto& object : objects) {
			result.push_back(object);
		}
		return result;
	}
}

void StageGimmickManager::Initialize() {

}

void StageGimmickManager::Update(float dt) {
	dt;
}

void StageGimmickManager::ShowGui() {

}

void StageGimmickManager::GimmickShowGui(const std::string& gimmickName) {

	ImGui::Separator();
	std::string addButtonLabel = "Add_" + gimmickName;
	if(ImGui::Button(addButtonLabel.c_str())) {
		CreateGimmick(gimmickName);
	}
	ImGui::Separator();

	for(int i = 0; i < gimmicks_.size(); ++i) {
		if(gimmickName != gimmicks_[i].name) { continue; }
		ImGui::PushID(i);
		std::string label = "Delete_" + gimmicks_[i].event->GetName();
		if(ImGui::Button(label.c_str())) {
			DeleteGimmick(i);
			ImGui::PopID();
			break;
		}

		ImGui::PopID();
	}
	ImGui::Separator();
}

void StageGimmickManager::ReloadGimmicks(const std::string& gimmickName) {
	auto* ctx = SceneContext::Current();
	if(!ctx) return;
	auto* lib = ctx->GetObjectLibrary();
	if(!lib) return;

	if(gimmickName == "BreakableFloor") {
		for(auto& event : FindObjectsByClassName<BreakableFloorEvent>(*lib, "BreakableFloorEvent")) {
			auto objects = FindChildrenByClassName<BreakableFloorObject>(*event, "BreakableFloorObject");
			if(objects.empty()) continue;
			event->SetTarget(objects.front());
			event->Initialize();
			gimmicks_.push_back({event, ToGimmickObjects(objects), gimmickName});
		}
	} else if(gimmickName == "GroundSpike") {
		for(auto& event : FindObjectsByClassName<GroundSpikeEvent>(*lib, "GroundSpikeEvent")) {
			auto objects = FindChildrenByClassName<GroundSpikeObject>(*event, "GroundSpikeObject");
			if(objects.empty()) continue;
			event->SetTarget(objects.front());
			event->Initialize();
			gimmicks_.push_back({event, ToGimmickObjects(objects), gimmickName});
		}
	} else if(gimmickName == "DroolRain") {
		for(auto& event : FindObjectsByClassName<DroolRainEvent>(*lib, "DroolRainEvent")) {
			auto objects = FindChildrenByClassName<DroolRainObject>(*event, "DroolRainObject");
			if(objects.empty()) continue;
			for(auto& object : objects) {
				event->SetTarget(object);
			}
			event->Initialize();
			gimmicks_.push_back({event, ToGimmickObjects(objects), gimmickName});
		}
	} else if(gimmickName == "Projectile") {
		for(auto& event : FindObjectsByClassName<ProjectileFireEvent>(*lib, "ProjectileFireEvent")) {
			auto objects = FindChildrenByClassName<ProjectileObject>(*event, "ProjectileObject");
			if(objects.empty()) continue;
			event->SetTarget(objects.front());
			event->Initialize();
			gimmicks_.push_back({event, ToGimmickObjects(objects), gimmickName});
		}
	}

}

void StageGimmickManager::CreateGimmick(const std::string& gimmickName) {

	// オブジェクトとイベントの名前を作成
	const std::string objectName = gimmickName + "Object";
	const std::string eventName	 = gimmickName + "Event";

	// オブジェクトとイベントをシーンに生成
	std::vector<std::shared_ptr<StageGimmickObjectBase>> objects;
	std::shared_ptr<StageGimmickEventBase> event;
	if(gimmickName == "BreakableFloor") {

		auto breakableObject = SceneAPI::Instantiate<BreakableFloorObject>("debugCube.obj", objectName);
		auto breakableEvent	 = SceneAPI::Instantiate<BreakableFloorEvent>(eventName);
		breakableEvent->SetTarget(breakableObject);
		breakableObject->SetParent(breakableEvent);
		breakableEvent->Initialize();
		breakableObject->Initialize();
		objects.push_back(breakableObject);
		event = breakableEvent;

	} else if(gimmickName == "GroundSpike") {

		auto spikeObject = SceneAPI::Instantiate<GroundSpikeObject>("debugCube.obj", objectName);
		auto spikeEvent	 = SceneAPI::Instantiate<GroundSpikeEvent>(eventName);
		spikeEvent->SetTarget(spikeObject);
		spikeObject->SetParent(spikeEvent);
		spikeEvent->Initialize();
		spikeObject->Initialize();
		objects.push_back(spikeObject);
		event = spikeEvent;

	} else if(gimmickName == "DroolRain") {
		auto droolEvent = SceneAPI::Instantiate<DroolRainEvent>(eventName);
		droolEvent->Initialize();
		event = droolEvent;
		for(uint32_t i = 0; i < event->GetObjectCount(); ++i) {
			auto droolObject = SceneAPI::Instantiate<DroolRainObject>("debugCube.obj", objectName);
			droolObject->SetParent(droolEvent);
			droolObject->Initialize();
			objects.push_back(droolObject);
			droolEvent->SetTarget(droolObject);
		}
	} else if(gimmickName == "Projectile") {

		auto projectileObject = SceneAPI::Instantiate<ProjectileObject>("debugCube.obj", objectName);
		auto projectileEvent = SceneAPI::Instantiate<ProjectileFireEvent>(eventName);
		projectileEvent->SetTarget(projectileObject);
		projectileObject->SetParent(projectileEvent);
		projectileEvent->Initialize();
		projectileObject->Initialize();
		objects.push_back(projectileObject);
		event = projectileEvent;
	}

	gimmicks_.push_back({event, objects, gimmickName});
	ReindexGimmickNames(gimmickName);
}

void StageGimmickManager::DeleteGimmick(size_t index) {
	// インデックスが範囲外なら何もしない
	if(index >= gimmicks_.size()) {
		return;
	}

	// シーンからオブジェクトとイベントを削除
	auto event   = gimmicks_[index].event;
	auto objects = gimmicks_[index].objects;
	const auto name = gimmicks_[index].name;

	// シーンコンテキストが存在する場合に削除処理を行う
	if(auto* ctx = SceneContext::Current()) {
		if(event) { ctx->RemoveObject(event); }
		for(auto& object : objects) {
			if(object) {
				ctx->RemoveObject(object);
			}
		}
	}

	// リストから削除
	gimmicks_.erase(gimmicks_.begin() + index);
	ReindexGimmickNames(name);
}

void StageGimmickManager::ReindexGimmickNames(const std::string& gimmickName) {

	const size_t n = gimmicks_.size();

	size_t index = 0;
	for(size_t i = 0; i < n; ++i) {
		if(gimmicks_[i].name != gimmickName) {
			continue;
		}
		if(gimmicks_[i].event) {
			gimmicks_[i].event->SetName(gimmickName + "Event", std::nullopt);
		}
		for(size_t objectIndex = 0; objectIndex < gimmicks_[i].objects.size(); ++objectIndex) {
			auto& object = gimmicks_[i].objects[objectIndex];
			if(!object) {
				continue;
			}
			object->SetName(gimmickName + "Object");
		}
		++index;
	}
}
