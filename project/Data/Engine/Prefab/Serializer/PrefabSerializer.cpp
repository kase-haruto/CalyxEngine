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

	return JsonUtils::Save(path, jArray);
}

std::vector<std::shared_ptr<SceneObject>> PrefabSerializer::Load(const std::string& path){
	nlohmann::json jArray;
	if (!JsonUtils::Load(path, jArray)) return {};

	std::vector<std::shared_ptr<SceneObject>> createdRoots;
	std::unordered_map<Guid, std::shared_ptr<SceneObject>> guidMap;

	// インスタンス生成と設定適用
	for (const auto& j : jArray){
		std::string typeName = j.value("type", "");
		if (typeName.empty()) continue;

		// Factory で生成
		auto sp = SceneObjectRegistry::Get().Create(typeName);
		if (!sp) continue;

		// Config 適用
		if (auto* cfg = dynamic_cast< IConfigurable* >(sp.get())){
			cfg->ApplyConfigFromJson(j);
		}

		// GUID 上書き
		Guid guid = j.value("guid", Guid {});
		sp->SetGuid(guid);

		guidMap[guid] = sp;
		createdRoots.push_back(sp);
	}

	// 親子リンク復元
	for (const auto& j : jArray){
		Guid childG = j.value("guid", Guid {});
		Guid parentG = j.value("parentGuid", Guid {});
		if (!childG.isValid() || !parentG.isValid()) continue;

		auto childIt = guidMap.find(childG);
		auto parentIt = guidMap.find(parentG);
		if (childIt != guidMap.end() && parentIt != guidMap.end()){
			childIt->second->SetParent(parentIt->second);
		}
	}

	// ルートだけを返
	std::vector<std::shared_ptr<SceneObject>> rootsOut;
	for (auto& [g, sp] : guidMap){
		if (!sp->GetParent()) rootsOut.push_back(sp);
	}
	return rootsOut;
}
