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
		config.translation			= CalyxEngine::Vector3::Zero();
		config.inheritTranslate		= true;
		config.inheritRotate		= true;
		config.inheritScale			= true;
		return config;
	}

	void WriteSceneObjectMetadata(SceneObject&							  obj,
								  nlohmann::json&						  j,
								  const std::unordered_set<SceneObject*>& prefabRoots,
								  bool									  resetRootTransform,
								  bool									  usePrefabSourceGuids) {
		j["type"]			  = std::string(obj.GetTypeName());
		const Guid guid		  = (usePrefabSourceGuids && obj.GetPrefabSourceGuid().isValid())
									? obj.GetPrefabSourceGuid()
									: obj.GetGuid();
		j["guid"]			  = guid;
		j["name"]			  = obj.GetName();
		j["objectType"]		  = static_cast<int>(obj.GetObjectType());
		j["drawEnable"]		  = obj.IsDrawEnable();
		j["castShadow"]		  = obj.IsCastShadow();
		j["cameraDitherEnabled"] = obj.IsCameraDitherEnabled();
		j["outlineEnabled"]	  = obj.IsOutlineEnabled();
		j["outlineThickness"] = obj.GetOutlineSettings().thickness;
		j["outlineColor"]	  = obj.GetOutlineSettings().color;
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
		obj.SetCameraDitherEnabled(j.value("cameraDitherEnabled", obj.IsCameraDitherEnabled()));
		obj.SetOutlineEnabled(j.value("outlineEnabled", obj.IsOutlineEnabled()));
		obj.SetOutlineThickness(j.value("outlineThickness", obj.GetOutlineSettings().thickness));
		obj.SetOutlineColor(j.value("outlineColor", obj.GetOutlineSettings().color));
		if(j.contains("worldTransform")) {
			obj.GetWorldTransform().ApplyConfig(j.at("worldTransform").get<WorldTransformConfig>());
		}
	}

	void CollectObjectsRecursive(SceneObject* obj, std::vector<SceneObject*>& out) {
		// TransientまたはRegistryで非対応の型を除外し、Prefabへ保存可能なHierarchyだけを収集する。
		if(!obj || obj->IsTransient() || !SceneObjectRegistry::Get().IsPrefabSerializable(obj->GetTypeName())) return;
		out.push_back(obj);
		for(const auto& child : obj->GetChildren()) {
			CollectObjectsRecursive(child.get(), out);
		}
	}

	nlohmann::json BuildBoneParentBindingsJson(
		const BaseGameObject& owner,
		const std::vector<SceneObject*>& objects,
		bool usePrefabSourceGuids) {
		nlohmann::json bindings = nlohmann::json::array();

		// WorldTransform pointerを保存可能なGUIDへ解決し、Bone追従関係をAssetへ変換する。
		for(const auto& binding : owner.GetBoneParentBindings()) {
			if(!binding.target || binding.boneName.empty()) continue;

			for(const auto* target : objects) {
				if(!target) continue;
				if(&target->GetWorldTransform() != binding.target) continue;

				const Guid targetGuid =
					(usePrefabSourceGuids && target->GetPrefabSourceGuid().isValid())
						? target->GetPrefabSourceGuid()
						: target->GetGuid();
				bindings.push_back(nlohmann::json{
					{"targetGuid", targetGuid},
					{"boneName", binding.boneName},
					{"inheritScale", binding.inheritScale}});
				break;
			}
		}

		return bindings;
	}

} // namespace

bool PrefabSerializer::Save(const std::vector<SceneObject*>& roots,
							const std::string&				 path) {
	return Save(roots, path, SaveOptions{});
}

bool PrefabSerializer::Save(const std::vector<SceneObject*>& roots,
							const std::string&				 path,
							const SaveOptions&				 options) {
	nlohmann::json					 jArray = nlohmann::json::array();
	std::unordered_set<SceneObject*> prefabRoots;
	std::unordered_map<Guid, Guid>	 prefabSourceGuidMap;
	std::vector<SceneObject*>		 prefabObjects;
	// 保存Rootと配下Objectを先に収集し、GUID RemapやBone参照解決の対象集合を確定する。
	for(auto* root : roots) {
		if(!root) continue;
		prefabRoots.insert(root);
		CollectObjectsRecursive(root, prefabObjects);
		if(options.usePrefabSourceGuids) {
			CollectPrefabSourceGuidMap(root, prefabSourceGuidMap);
		}
	}

	// 親を先に出力する深さ優先順で、Config・Metadata・Serializable Paramを統合する。
	std::function<void(SceneObject*)> serializeRec;
	serializeRec = [&](SceneObject* obj) {
		if(!obj || obj->IsTransient() || !SceneObjectRegistry::Get().IsPrefabSerializable(obj->GetTypeName())) return;

		nlohmann::json j;
		if(auto* cfg = dynamic_cast<IConfigurable*>(obj)) {
			cfg->ExtractConfigToJson(j);
		}
		WriteSceneObjectMetadata(*obj, j, prefabRoots, options.resetRootTransform, options.usePrefabSourceGuids);
		// BaseGameObject固有のBone Bindingは対象GUIDへ変換して別項目として保存する。
		if(auto* owner = dynamic_cast<BaseGameObject*>(obj)) {
			auto bindings = BuildBoneParentBindingsJson(*owner, prefabObjects, options.usePrefabSourceGuids);
			if(!bindings.empty()) {
				j["boneParentBindings"] = std::move(bindings);
			}
		}
		nlohmann::json serializableParams;
		obj->ExtractSerializableParamsToJson(serializableParams);
		if(!serializableParams.empty()) {
			j["serializableParams"] = std::move(serializableParams);
		}
		// InstanceからApplyする場合はScene GUIDをSource GUIDへ戻し、Asset内部参照を安定化する。
		if(options.usePrefabSourceGuids && !prefabSourceGuidMap.empty()) {
			RemapJsonGuidStrings(j, prefabSourceGuidMap);
		}
		jArray.push_back(std::move(j));

		for(auto& childSp : obj->GetChildren()) {
			if(childSp) serializeRec(childSp.get());
		}
	};

	for(auto* root : roots) serializeRec(root);

	return CalyxEngine::JsonUtils::Save(path, jArray);
}

std::vector<std::shared_ptr<SceneObject>> PrefabSerializer::Load(const std::string& path) {
	return Load(path, LoadOptions{});
}

std::vector<std::shared_ptr<SceneObject>> PrefabSerializer::Load(const std::string& path,
																 const LoadOptions& options) {
	nlohmann::json jArray;
	if(!CalyxEngine::JsonUtils::Load(path, jArray)) return {};

	// 保存GUID、新規GUID、生成Objectの対応を保持し、内部参照とHierarchyを後段で復元する。
	std::unordered_map<Guid, std::shared_ptr<SceneObject>> oldToObject;
	std::unordered_map<Guid, Guid>						   oldToNewGuid;
	std::unordered_map<Guid, std::shared_ptr<SceneObject>> guidMap;

	// 第一段階で全Instanceを生成し、設定適用と新GUID割当まで完了させる。
	for(const auto& j : jArray) {
		std::string typeName = j.value("type", "");
		if(typeName.empty()) continue;

		const nlohmann::json* paramOverrides = j.contains("serializableParams")
												   ? &j.at("serializableParams")
												   : nullptr;
		// Constructor登録Fieldへ保存値を渡すため、Registry生成前にPending Captureを開始する。
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
		if(!sp) {
			CalyxEngine::SerializableObject::EndPendingCapture(nullptr, nullptr);
			continue;
		}
		sp->AdoptPendingSerializableParamCapture(paramOverrides);

		// Resource生成前にConfigを適用し、Initializeが保存状態を基準に動作できるようにする。
		if(auto* cfg = dynamic_cast<IConfigurable*>(sp.get())) {
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
		// Initialize中に読み出されるSerializable ParamをCapture範囲内で供給する。
		sp->BeginSerializableParamCapture(paramOverrides);
		sp->Initialize();
		sp->EndSerializableParamCapture();
		if(auto* cfg = dynamic_cast<IConfigurable*>(sp.get())) {
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
		oldToObject[oldGuid]  = sp;
		guidMap[newGuid]	  = sp;
	}

	// 全Object生成後にSceneObject参照GUIDを新GUIDへ一括変換する。
	for(auto& [oldGuid, sp] : oldToObject) {
		(void)oldGuid;
		if(sp) {
			sp->RemapSceneObjectReferences(oldToNewGuid);
		}
	}

	// 第二段階で新GUID Mapを使って親子関係を復元し、生成順への依存をなくす。
	for(const auto& j : jArray) {
		Guid oldChild  = j.value("guid", Guid{});
		Guid oldParent = j.value("parentGuid", Guid{});

		auto newChildIt	 = oldToNewGuid.find(oldChild);
		auto newParentIt = oldToNewGuid.find(oldParent);
		if(newChildIt == oldToNewGuid.end()) continue;

		auto childMapIt = guidMap.find(newChildIt->second);
		if(childMapIt == guidMap.end()) continue;
		auto childSp = childMapIt->second;
		if(!childSp) continue;

		if(newParentIt != oldToNewGuid.end()) {
			auto parentMapIt = guidMap.find(newParentIt->second);
			if(parentMapIt == guidMap.end()) continue;
			auto parentSp = parentMapIt->second;
			if(parentSp) {
				// children一覧はSetParent内部で更新されるため、片側だけを直接変更しない。
				auto& childTransform = childSp->GetWorldTransform();
				childSp->SetParent(parentSp, childTransform.inheritScale);
			}
		}
	}

	// 第三段階でBone Bindingを復元し、対象Transformがすべて生成済みであることを保証する。
	for(const auto& j : jArray) {
		Guid oldOwner = j.value("guid", Guid{});
		if(!oldOwner.isValid() || !j.contains("boneParentBindings")) continue;

		auto newOwnerIt = oldToNewGuid.find(oldOwner);
		if(newOwnerIt == oldToNewGuid.end()) continue;

		auto ownerMapIt = guidMap.find(newOwnerIt->second);
		if(ownerMapIt == guidMap.end()) continue;
		auto  ownerSp = ownerMapIt->second;
		auto* owner	  = dynamic_cast<BaseGameObject*>(ownerSp.get());
		if(!owner) continue;

		for(const auto& bindingJson : j.at("boneParentBindings")) {
			Guid		oldTarget = bindingJson.value("targetGuid", Guid{});
			std::string boneName  = bindingJson.value("boneName", std::string{});
			if(!oldTarget.isValid() || boneName.empty()) continue;

			auto newTargetIt = oldToNewGuid.find(oldTarget);
			if(newTargetIt == oldToNewGuid.end()) continue;

			auto targetMapIt = guidMap.find(newTargetIt->second);
			if(targetMapIt == guidMap.end()) continue;
			auto targetSp = targetMapIt->second;
			if(!targetSp) continue;

			owner->SetBoneParent(
				targetSp->GetWorldTransform(),
				boneName,
				bindingJson.value("inheritScale", true));
		}
	}

	// 呼び出し側がSceneContextへ全Hierarchyを登録できるよう、Root以外も返却する。
	std::vector<std::shared_ptr<SceneObject>> allObjects;
	allObjects.reserve(guidMap.size());
	for(auto& [g, sp] : guidMap) {
		allObjects.push_back(sp);
	}
	return allObjects;
}
