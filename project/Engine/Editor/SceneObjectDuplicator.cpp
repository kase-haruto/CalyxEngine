#include "SceneObjectDuplicator.h"

#include <Data/Engine/Configs/Scene/Objects/Transform/WorldTransformConfig.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <algorithm>
#include <externals/nlohmann/json.hpp>

namespace CalyxEngine {
	namespace {
		void WriteMetadata(const std::shared_ptr<SceneObject>& object, nlohmann::json& j) {
			j["type"] = std::string(object->GetTypeName());
			j["name"] = object->GetName();
			j["objectType"] = static_cast<int>(object->GetObjectType());
			j["drawEnable"] = object->IsDrawEnable();
			j["castShadow"] = object->IsCastShadow();
			j["cameraDitherEnabled"] = object->IsCameraDitherEnabled();
			j["outlineEnabled"] = object->IsOutlineEnabled();
			j["outlineThickness"] = object->GetOutlineSettings().thickness;
			j["outlineColor"] = object->GetOutlineSettings().color;
			j["worldTransform"] = object->GetWorldTransform().ExtractConfig();
			j["inheritScale"] = object->GetWorldTransform().inheritScale;
		}

		void ApplyMetadata(const std::shared_ptr<SceneObject>& object, const nlohmann::json& j) {
			if(!object) return;

			const int objectType = j.value("objectType", static_cast<int>(object->GetObjectType()));
			object->SetName(j.value("name", object->GetName()), static_cast<ObjectType>(objectType));
			object->SetDrawEnable(j.value("drawEnable", object->IsDrawEnable()));
			object->SetCastShadow(j.value("castShadow", object->IsCastShadow()));
			object->SetCameraDitherEnabled(j.value("cameraDitherEnabled", object->IsCameraDitherEnabled()));
			object->SetOutlineEnabled(j.value("outlineEnabled", object->IsOutlineEnabled()));
			object->SetOutlineThickness(j.value("outlineThickness", object->GetOutlineSettings().thickness));
			object->SetOutlineColor(j.value("outlineColor", object->GetOutlineSettings().color));

			if(j.contains("worldTransform")) {
				object->GetWorldTransform().ApplyConfig(j.at("worldTransform").get<WorldTransformConfig>());
			}
		}

		std::shared_ptr<SceneObject> CloneWithoutHierarchy(const std::shared_ptr<SceneObject>& source) {
			if(!SceneObjectDuplicator::IsDuplicatable(source.get())) return nullptr;

			nlohmann::json snapshot;
			WriteMetadata(source, snapshot);
			if(auto* cfg = dynamic_cast<const IConfigurable*>(source.get())) {
				nlohmann::json config;
				cfg->ExtractConfigToJson(config);
				for(auto it = config.begin(); it != config.end(); ++it) {
					snapshot[it.key()] = it.value();
				}
			}
			nlohmann::json serializableParams;
			source->ExtractSerializableParamsToJson(serializableParams);
			if(!serializableParams.empty()) {
				snapshot["serializableParams"] = std::move(serializableParams);
			}

			std::shared_ptr<SceneObject> clone;
			const nlohmann::json* paramOverrides = snapshot.contains("serializableParams")
				? &snapshot.at("serializableParams")
				: nullptr;
			try {
				SerializableObject::BeginPendingCapture();
				clone = SceneObjectRegistry::Get().Create(snapshot.value("type", ""));
			} catch(...) {
				SerializableObject::EndPendingCapture(nullptr, nullptr);
				return nullptr;
			}
			if(!clone || !SceneObjectDuplicator::IsDuplicatable(clone.get())) {
				SerializableObject::EndPendingCapture(nullptr, nullptr);
				return nullptr;
			}
			clone->AdoptPendingSerializableParamCapture(paramOverrides);

			if(auto* cfg = dynamic_cast<IConfigurable*>(clone.get())) {
				cfg->ApplyConfigFromJson(snapshot);
			}
			ApplyMetadata(clone, snapshot);
			clone->SetGuid(Guid::New());
			clone->ClearPrefabLink();
			clone->SetPickingID(0);
			clone->BeginSerializableParamCapture(paramOverrides);
			clone->Initialize();
			clone->EndSerializableParamCapture();
			return clone;
		}

		void CollectDuplicateRoots(const std::vector<std::shared_ptr<SceneObject>>& selected,
								   std::vector<std::shared_ptr<SceneObject>>& roots) {
			for(const auto& object : selected) {
				if(!SceneObjectDuplicator::IsDuplicatable(object.get())) continue;

				bool hasSelectedDuplicatableAncestor = false;
				for(auto parent = object->GetParent(); parent; parent = parent->GetParent()) {
					if(!SceneObjectDuplicator::IsDuplicatable(parent.get())) continue;
					if(std::find(selected.begin(), selected.end(), parent) != selected.end()) {
						hasSelectedDuplicatableAncestor = true;
						break;
					}
				}
				if(!hasSelectedDuplicatableAncestor) {
					roots.push_back(object);
				}
			}
		}

		std::shared_ptr<SceneObject> DuplicateSubtree(
			SceneContext* ctx,
			const std::shared_ptr<SceneObject>& source,
			const std::shared_ptr<SceneObject>& duplicateParent,
			std::vector<std::shared_ptr<SceneObject>>& created) {
			if(!ctx || !SceneObjectDuplicator::IsDuplicatable(source.get())) return nullptr;

			auto clone = CloneWithoutHierarchy(source);
			if(!clone) return nullptr;

			if(duplicateParent) {
				clone->SetParent(duplicateParent, source->GetWorldTransform().inheritScale);
			} else if(auto parent = source->GetParent()) {
				clone->SetParent(parent, source->GetWorldTransform().inheritScale);
			}

			ctx->AddObject(clone);
			created.push_back(clone);

			for(const auto& child : source->GetChildren()) {
				if(SceneObjectDuplicator::IsDuplicatable(child.get())) {
					DuplicateSubtree(ctx, child, clone, created);
				}
			}
			return clone;
		}
	} // namespace

	bool SceneObjectDuplicator::IsDuplicatable(const SceneObject* object) {
		if(!object || object->IsTransient()) return false;
		return object->GetObjectType() == ObjectType::GameObject ||
			   object->GetObjectType() == ObjectType::Event;
	}

	std::vector<std::shared_ptr<SceneObject>> SceneObjectDuplicator::FilterDuplicatable(
		const std::vector<std::shared_ptr<SceneObject>>& objects) {
		std::vector<std::shared_ptr<SceneObject>> filtered;
		filtered.reserve(objects.size());
		for(const auto& object : objects) {
			if(IsDuplicatable(object.get())) {
				filtered.push_back(object);
			}
		}
		return filtered;
	}

	SceneObjectDuplicateResult SceneObjectDuplicator::Duplicate(
		SceneContext* ctx,
		const std::vector<std::shared_ptr<SceneObject>>& sources) {
		SceneObjectDuplicateResult result;
		if(!ctx) return result;

		std::vector<std::shared_ptr<SceneObject>> roots;
		std::vector<std::shared_ptr<SceneObject>> allCreated;
		CollectDuplicateRoots(sources, roots);

		for(const auto& root : roots) {
			auto duplicateRoot = DuplicateSubtree(ctx, root, nullptr, allCreated);
			if(duplicateRoot) {
				result.rootGuids.push_back(duplicateRoot->GetGuid());
				result.selectedRoots.push_back(duplicateRoot);
			}
		}
		return result;
	}

} // namespace CalyxEngine
