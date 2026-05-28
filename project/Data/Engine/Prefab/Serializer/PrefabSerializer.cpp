#include "PrefabSerializer.h"
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Foundation/Json/JsonUtils.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>
#include <Engine/Objects/LightObject/PointLight.h>

#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace {

	bool IsGuidString(const std::string& value) {
		if(value.size() != 36) return false;
		for(size_t i = 0; i < value.size(); ++i) {
			const char c = value[i];
			if(i == 8 || i == 13 || i == 18 || i == 23) {
				if(c != '-') return false;
				continue;
			}

			const bool isHex =
				(c >= '0' && c <= '9') ||
				(c >= 'a' && c <= 'f') ||
				(c >= 'A' && c <= 'F');
			if(!isHex) return false;
		}
		return true;
	}

	void RemapJsonGuidStrings(nlohmann::json& j, const std::unordered_map<Guid, Guid>& guidMap) {
		if(j.is_object()) {
			for(auto& item : j.items()) {
				RemapJsonGuidStrings(item.value(), guidMap);
			}
			return;
		}

		if(j.is_array()) {
			for(auto& item : j) {
				RemapJsonGuidStrings(item, guidMap);
			}
			return;
		}

		if(!j.is_string()) return;

		const std::string value = j.get<std::string>();
		if(!IsGuidString(value)) return;

		const Guid guid = Guid::FromString(value);
		if(auto it = guidMap.find(guid); it != guidMap.end() && it->second.isValid()) {
			j = it->second;
		}
	}

	void CollectPrefabSourceGuidMap(SceneObject* obj, std::unordered_map<Guid, Guid>& out) {
		if(!obj) return;

		const Guid& sourceGuid = obj->GetPrefabSourceGuid();
		if(obj->GetGuid().isValid() && sourceGuid.isValid()) {
			out[obj->GetGuid()] = sourceGuid;
		}

		for(const auto& child : obj->GetChildren()) {
			CollectPrefabSourceGuidMap(child.get(), out);
		}
	}

	WorldTransformConfig MakePrefabRootTransformConfig(WorldTransform& transform) {
		WorldTransformConfig config = transform.ExtractConfig();
		config.translation = CalyxEngine::Vector3::Zero();
		config.inheritTranslate = true;
		config.inheritRotate = true;
		config.inheritScale = true;
		return config;
	}

	void WriteSceneObjectMetadata(SceneObject& obj,
								  nlohmann::json& j,
								  const std::unordered_set<SceneObject*>& prefabRoots,
								  bool resetRootTransform,
								  bool usePrefabSourceGuids) {
		j["type"] = obj.GetObjectClassName();
		const Guid guid = (usePrefabSourceGuids && obj.GetPrefabSourceGuid().isValid())
			? obj.GetPrefabSourceGuid()
			: obj.GetGuid();
		j["guid"] = guid;
		j["name"] = obj.GetName();
		j["objectType"] = static_cast<int>(obj.GetObjectType());
		j["drawEnable"] = obj.IsDrawEnable();
		j["castShadow"] = obj.IsCastShadow();
		j["outlineEnabled"] = obj.IsOutlineEnabled();
		j["outlineThickness"] = obj.GetOutlineSettings().thickness;
		j["outlineColor"] = obj.GetOutlineSettings().color;
		if(resetRootTransform && prefabRoots.contains(&obj)) {
			j["worldTransform"] = MakePrefabRootTransformConfig(obj.GetWorldTransform());
		} else {
			j["worldTransform"] = obj.GetWorldTransform().ExtractConfig();
		}
		if(auto parent = obj.GetParent()) {
			if(usePrefabSourceGuids && parent->GetPrefabSourceGuid().isValid()) {
				j["parentGuid"] = parent->GetPrefabSourceGuid();
			} else {
				j["parentGuid"] = parent->GetGuid();
			}
		} else {
			j["parentGuid"] = Guid::Empty();
		}
	}

	void ApplySceneObjectMetadata(SceneObject& obj, const nlohmann::json& j) {
		const int objectType = j.value("objectType", static_cast<int>(obj.GetObjectType()));
		obj.SetName(j.value("name", obj.GetName()), static_cast<ObjectType>(objectType));
		if(obj.GetObjectClassName() == std::string_view("SceneObject")) {
			obj.SetDrawEnable(j.value("drawEnable", obj.IsDrawEnable()));
		}
		obj.SetCastShadow(j.value("castShadow", obj.IsCastShadow()));
		obj.SetOutlineEnabled(j.value("outlineEnabled", obj.IsOutlineEnabled()));
		obj.SetOutlineThickness(j.value("outlineThickness", obj.GetOutlineSettings().thickness));
		obj.SetOutlineColor(j.value("outlineColor", obj.GetOutlineSettings().color));
		if(j.contains("worldTransform")) {
			obj.GetWorldTransform().ApplyConfig(j.at("worldTransform").get<WorldTransformConfig>());
		}
	}

} // namespace

bool PrefabSerializer::Save(const std::vector<SceneObject*>& roots,
							const std::string& path){
	return Save(roots, path, SaveOptions{});
}

bool PrefabSerializer::Save(const std::vector<SceneObject*>& roots,
							const std::string& path,
							const SaveOptions& options){
	nlohmann::json jArray = nlohmann::json::array();
	std::unordered_set<SceneObject*> prefabRoots;
	std::unordered_map<Guid, Guid> prefabSourceGuidMap;
	for(auto* root : roots) {
		if(!root) continue;
		prefabRoots.insert(root);
		if(options.usePrefabSourceGuids) {
			CollectPrefabSourceGuidMap(root, prefabSourceGuidMap);
		}
	}

	std::function<void(SceneObject*)> serializeRec;
	serializeRec = [&] (SceneObject* obj){
		if (!obj || !obj->IsSerializable()) return;

		nlohmann::json j;
		if (auto* cfg = dynamic_cast< IConfigurable* >(obj)){
			cfg->ExtractConfigToJson(j);
		}
		WriteSceneObjectMetadata(*obj, j, prefabRoots, options.resetRootTransform, options.usePrefabSourceGuids);
		nlohmann::json serializableParams;
		obj->ExtractSerializableParamsToJson(serializableParams);
		if(!serializableParams.empty()) {
			j["serializableParams"] = std::move(serializableParams);
		}
		if(options.usePrefabSourceGuids && !prefabSourceGuidMap.empty()) {
			RemapJsonGuidStrings(j, prefabSourceGuidMap);
		}
		jArray.push_back(std::move(j));

		for (auto& childSp : obj->GetChildren()){
			if (childSp) serializeRec(childSp.get());
		}
		};

	for (auto* root : roots) serializeRec(root);

	return CalyxEngine::JsonUtils::Save(path, jArray);
}

std::vector<std::shared_ptr<SceneObject>> PrefabSerializer::Load(const std::string& path) {
	return Load(path, LoadOptions{});
}

std::vector<std::shared_ptr<SceneObject>> PrefabSerializer::Load(const std::string& path,
																 const LoadOptions& options) {
	nlohmann::json jArray;
	if (!CalyxEngine::JsonUtils::Load(path, jArray)) return {};

	std::unordered_map<Guid, std::shared_ptr<SceneObject>> oldToObject;
	std::unordered_map<Guid, Guid> oldToNewGuid;
	std::unordered_map<Guid, std::shared_ptr<SceneObject>> guidMap;

	// 1. インスタンス生成と設定適用 + 新 GUID 割り当て
	for (const auto& j : jArray) {
		std::string typeName = j.value("type", "");
		if (typeName.empty()) continue;

		const nlohmann::json* paramOverrides = j.contains("serializableParams")
			? &j.at("serializableParams")
			: nullptr;
		CalyxEngine::SerializableObject::BeginPendingCapture();
		std::shared_ptr<SceneObject> sp;
		try {
			sp = SceneObjectRegistry::Get().Create(typeName);
		} catch(...) {
			CalyxEngine::SerializableObject::EndPendingCapture(nullptr, nullptr);
			if(options.skipUnknownTypes) {
				continue;
			}
			throw;
		}
		if (!sp) {
			CalyxEngine::SerializableObject::EndPendingCapture(nullptr, nullptr);
			continue;
		}
		sp->AdoptPendingSerializableParamCapture(paramOverrides);

		if (auto* cfg = dynamic_cast<IConfigurable*>(sp.get())) {
			cfg->ApplyConfigFromJson(j);
		}
		ApplySceneObjectMetadata(*sp, j);

		Guid oldGuid = j.value("guid", Guid{});
		Guid newGuid = options.preserveGuids ? oldGuid : Guid::New();
		if(!newGuid.isValid()) {
			newGuid = Guid::New();
		}
		sp->SetGuid(newGuid);
		if(options.prefabAssetGuid.isValid() && oldGuid.isValid()) {
			sp->SetPrefabLink(options.prefabAssetGuid, oldGuid);
		} else if(options.preserveGuids) {
			sp->ClearPrefabLink();
		}
		sp->BeginSerializableParamCapture(paramOverrides);
		sp->Initialize();
		sp->EndSerializableParamCapture();
		if (auto* cfg = dynamic_cast<IConfigurable*>(sp.get())) {
			cfg->ApplyConfigFromJson(j);
			ApplySceneObjectMetadata(*sp, j);
			sp->SetGuid(newGuid);
			if(options.prefabAssetGuid.isValid() && oldGuid.isValid()) {
				sp->SetPrefabLink(options.prefabAssetGuid, oldGuid);
			} else if(options.preserveGuids) {
				sp->ClearPrefabLink();
			}
		}

		oldToNewGuid[oldGuid] = newGuid;
		oldToObject[oldGuid] = sp;
		guidMap[newGuid] = sp;
	}

	for(auto& [oldGuid, sp] : oldToObject) {
		(void)oldGuid;
		if(sp) {
			sp->RemapSceneObjectReferences(oldToNewGuid);
		}
	}

	// 親子リンク復元
	for (const auto& j : jArray) {
		Guid oldChild = j.value("guid", Guid{});
		Guid oldParent = j.value("parentGuid", Guid{});

		auto newChildIt = oldToNewGuid.find(oldChild);
		auto newParentIt = oldToNewGuid.find(oldParent);
		if (newChildIt == oldToNewGuid.end()) continue;

		auto childMapIt = guidMap.find(newChildIt->second);
		if(childMapIt == guidMap.end()) continue;
		auto childSp = childMapIt->second;
		if(!childSp) continue;

		if (newParentIt != oldToNewGuid.end()) {
			auto parentMapIt = guidMap.find(newParentIt->second);
			if(parentMapIt == guidMap.end()) continue;
			auto parentSp = parentMapIt->second;
			if (parentSp) {
				// SetParent のみ（children_ は内部で処理される想定）
				auto& childTransform = childSp->GetWorldTransform();
				childSp->SetParent(parentSp, childTransform.inheritScale);
			}
		}
	}

	for(const auto& j : jArray) {
		Guid oldOwner = j.value("guid", Guid{});
		if(!oldOwner.isValid() || !j.contains("boneParentBindings")) continue;

		auto newOwnerIt = oldToNewGuid.find(oldOwner);
		if(newOwnerIt == oldToNewGuid.end()) continue;

		auto ownerMapIt = guidMap.find(newOwnerIt->second);
		if(ownerMapIt == guidMap.end()) continue;
		auto ownerSp = ownerMapIt->second;
		auto* owner = dynamic_cast<BaseGameObject*>(ownerSp.get());
		if(!owner) continue;

		for(const auto& bindingJson : j.at("boneParentBindings")) {
			Guid oldTarget = bindingJson.value("targetGuid", Guid{});
			std::string boneName = bindingJson.value("boneName", std::string{});
			if(!oldTarget.isValid() || boneName.empty()) continue;

			auto newTargetIt = oldToNewGuid.find(oldTarget);
			if(newTargetIt == oldToNewGuid.end()) continue;

			auto targetMapIt = guidMap.find(newTargetIt->second);
			if(targetMapIt == guidMap.end()) continue;
			auto targetSp = targetMapIt->second;
			auto* target = dynamic_cast<BaseGameObject*>(targetSp.get());
			if(!target) continue;

			owner->SetBoneParent(
				*target,
				boneName,
				bindingJson.value("inheritScale", true));
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
