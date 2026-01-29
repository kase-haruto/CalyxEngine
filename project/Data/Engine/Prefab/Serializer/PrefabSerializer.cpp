#include "PrefabSerializer.h"
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Foundation/Json/JsonUtils.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>
#include <Engine/Objects/LightObject/PointLight.h>

#include <functional>
#include <unordered_map>

bool PrefabSerializer::Save(const std::vector<SceneObject*>& roots,
							const std::string& path){
	nlohmann::json jArray = nlohmann::json::array();

	std::function<void(SceneObject*)> serializeRec;
	serializeRec = [&] (SceneObject* obj){
		if (!obj || !obj->IsSerializable()) return;

		if (auto* cfg = dynamic_cast< IConfigurable* >(obj)){
			nlohmann::json j;
			cfg->ExtractConfigToJson(j);
			j["type"] = obj->GetTypeName();  // 型名を保存
			j["guid"] = obj->GetGuid();
			if (auto parent = obj->GetParent()){
				j["parentGuid"] = parent->GetGuid();
			}
			jArray.push_back(std::move(j));
		}
		for (auto& childSp : obj->GetChildren()){
			if (childSp) serializeRec(childSp.get());
		}
		};

	for (auto* root : roots) serializeRec(root);

	return CalyxUtil::JsonUtils::Save(path, jArray);
}

std::vector<std::shared_ptr<SceneObject>> PrefabSerializer::Load(const std::string& path) {
	nlohmann::json jArray;
	if (!CalyxUtil::JsonUtils::Load(path, jArray)) return {};

	std::unordered_map<Guid, std::shared_ptr<SceneObject>> oldToObject;
	std::unordered_map<Guid, Guid> oldToNewGuid;
	std::unordered_map<Guid, std::shared_ptr<SceneObject>> guidMap;

	// 1. インスタンス生成と設定適用 + 新 GUID 割り当て
	for (const auto& j : jArray) {
		std::string typeName = j.value("type", "");
		if (typeName.empty()) continue;

		auto sp = SceneObjectRegistry::Get().Create(typeName);
		if (!sp) continue;

		if (auto* cfg = dynamic_cast<IConfigurable*>(sp.get())) {
			cfg->ApplyConfigFromJson(j);
		}

		Guid oldGuid = j.value("guid", Guid{});
		Guid newGuid = Guid::New();
		sp->SetGuid(newGuid);

		oldToNewGuid[oldGuid] = newGuid;
		oldToObject[oldGuid] = sp;
		guidMap[newGuid] = sp;
	}

	// 親子リンク復元
	for (const auto& j : jArray) {
		Guid oldChild = j.value("guid", Guid{});
		Guid oldParent = j.value("parentGuid", Guid{});

		auto newChildIt = oldToNewGuid.find(oldChild);
		auto newParentIt = oldToNewGuid.find(oldParent);
		if (newChildIt == oldToNewGuid.end()) continue;

		auto childSp = guidMap[newChildIt->second];
		if (!childSp) continue;

		if (newParentIt != oldToNewGuid.end()) {
			auto parentSp = guidMap[newParentIt->second];
			if (parentSp) {
				// SetParent のみ（children_ は内部で処理される想定）
				childSp->SetParent(parentSp);
			}
		}
	}

	// ルートだけでなく、すべてのオブジェクトを返す
	std::vector<std::shared_ptr<SceneObject>> allObjects;
	allObjects.reserve(guidMap.size());
	for (auto& [g, sp] : guidMap) {
		allObjects.push_back(sp);
	}
	return allObjects;
}