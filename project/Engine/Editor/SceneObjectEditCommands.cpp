#include "SceneObjectEditCommands.h"

#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Camera/3d/DebugCamera.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Lighting/LightLibrary.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Objects/LightObject/DirectionalLight.h>
#include <Engine/Objects/LightObject/PointLight.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/System/Command/Interface/ICommand.h>

#include <externals/nlohmann/json.hpp>

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace CalyxEngine {

	namespace {
		nlohmann::json CaptureObjectJson(const std::shared_ptr<SceneObject>& object) {
			nlohmann::json j;
			if(!object) return j;

			j["type"] = object->GetObjectClassName();
			j["guid"] = object->GetGuid();
			j["name"] = object->GetName();
			j["inheritScale"] = object->GetWorldTransform().inheritScale;
			if(auto parent = object->GetParent()) {
				j["parentGuid"] = parent->GetGuid();
			}
			const std::string& configPath = object->GetConfigPath();
			if(!configPath.empty()) {
				j["configPath"] = configPath;
			}

			if(auto* cfg = dynamic_cast<const IConfigurable*>(object.get())) {
				nlohmann::json inlineConfig;
				cfg->ExtractConfigToJson(inlineConfig);
				for(auto it = inlineConfig.begin(); it != inlineConfig.end(); ++it) {
					j[it.key()] = it.value();
				}
			}
			nlohmann::json serializableParams;
			object->ExtractSerializableParamsToJson(serializableParams);
			if(!serializableParams.empty()) {
				j["serializableParams"] = std::move(serializableParams);
			}
			return j;
		}

		void CaptureObjectTree(const std::shared_ptr<SceneObject>& object,
							   std::vector<nlohmann::json>& snapshots,
							   std::vector<Guid>& captured) {
			if(!object) return;
			if(std::find(captured.begin(), captured.end(), object->GetGuid()) != captured.end()) {
				return;
			}

			captured.push_back(object->GetGuid());
			snapshots.push_back(CaptureObjectJson(object));
			for(const auto& child : object->GetChildren()) {
				CaptureObjectTree(child, snapshots, captured);
			}
		}

		bool HasSelectedAncestor(const std::shared_ptr<SceneObject>& object,
								 const std::vector<Guid>& selectedGuids) {
			if(!object) return false;
			auto parent = object->GetParent();
			while(parent) {
				if(std::find(selectedGuids.begin(), selectedGuids.end(), parent->GetGuid()) != selectedGuids.end()) {
					return true;
				}
				parent = parent->GetParent();
			}
			return false;
		}

		void RegisterRestoredObject(SceneContext* ctx, const std::shared_ptr<SceneObject>& object) {
			if(!ctx || !object) return;

			if(auto dir = std::dynamic_pointer_cast<DirectionalLight>(object)) {
				ctx->GetLightLibrary()->SetDirectionalLight(dir);
			} else if(auto point = std::dynamic_pointer_cast<PointLight>(object)) {
				ctx->GetLightLibrary()->AddPointLight(point);
			} else if(auto debugCamera = std::dynamic_pointer_cast<DebugCamera>(object)) {
				ctx->GetCameraMgr()->SetDebugCamera(debugCamera);
			} else if(auto camera = std::dynamic_pointer_cast<Camera3d>(object)) {
				ctx->GetCameraMgr()->SetMainCamera(camera);
			}
		}

		class DeleteSceneObjectsCommand final
			: public ICommand {
		public:
			DeleteSceneObjectsCommand(SceneContext* ctx,
									  std::vector<std::shared_ptr<SceneObject>> targets,
									  SceneObjectEditCommandCallbacks callbacks,
									  std::string name)
				: ctx_(ctx),
				  callbacks_(std::move(callbacks)),
				  name_(std::move(name)) {
				std::vector<Guid> selectedGuids;
				selectedGuids.reserve(targets.size());
				for(const auto& target : targets) {
					if(target) selectedGuids.push_back(target->GetGuid());
				}

				std::vector<Guid> captured;
				for(auto& target : targets) {
					if(!target || HasSelectedAncestor(target, selectedGuids)) continue;
					rootGuids_.push_back(target->GetGuid());
					CaptureObjectTree(target, snapshots_, captured);
				}
			}

			void Execute() override {
				DeleteRoots();
				NotifyApplied();
			}

			void Undo() override {
				RestoreSnapshots();
				NotifyApplied();
			}

			void Redo() override {
				DeleteRoots();
				NotifyApplied();
			}

			const char* GetName() const override {
				return name_.c_str();
			}

		private:
			void NotifyApplied() {
				if(callbacks_.clearSelection) callbacks_.clearSelection();
				if(callbacks_.refreshHierarchy) callbacks_.refreshHierarchy();
			}

			void DeleteRoots() {
				if(!ctx_ || !ctx_->GetObjectLibrary()) return;
				for(const auto& guid : rootGuids_) {
					auto object = ctx_->GetObjectLibrary()->Find(guid);
					if(object) {
						ctx_->RemoveObject(object);
					}
				}
			}

			void RestoreSnapshots() {
				if(!ctx_ || !ctx_->GetObjectLibrary()) return;

				std::unordered_map<Guid, std::shared_ptr<SceneObject>> guidMap;
				for(const auto& j : snapshots_) {
					std::string typeName = j.value("type", "");
					if(typeName.empty()) continue;

					const nlohmann::json* paramOverrides = j.contains("serializableParams")
						? &j.at("serializableParams")
						: nullptr;
					SerializableObject::BeginPendingCapture();
					auto object = SceneObjectRegistry::Get().Create(typeName);
					if(!object) {
						SerializableObject::EndPendingCapture(nullptr, nullptr);
						continue;
					}
					object->AdoptPendingSerializableParamCapture(paramOverrides);

					if(j.contains("configPath")) {
						object->SetConfigPath(j.at("configPath").get<std::string>());
					}
					if(auto* cfg = dynamic_cast<IConfigurable*>(object.get())) {
						cfg->ApplyConfigFromJson(j);
					}

					const Guid guid = j.value("guid", Guid{});
					if(guid.isValid()) {
						object->SetGuid(guid);
					}

					ctx_->AddObject(object);
					object->BeginSerializableParamCapture(paramOverrides);
					object->Initialize();
					object->EndSerializableParamCapture();
					object->SetName(j.value("name", object->GetName()), object->GetObjectType());
					RegisterRestoredObject(ctx_, object);

					guidMap[object->GetGuid()] = object;
				}

				for(const auto& j : snapshots_) {
					const Guid childGuid = j.value("guid", Guid{});
					const Guid parentGuid = j.value("parentGuid", Guid{});
					if(!childGuid.isValid() || !parentGuid.isValid()) continue;

					auto childIt = guidMap.find(childGuid);
					if(childIt == guidMap.end()) continue;

					std::shared_ptr<SceneObject> parent;
					auto parentIt = guidMap.find(parentGuid);
					if(parentIt != guidMap.end()) {
						parent = parentIt->second;
					} else {
						parent = ctx_->GetObjectLibrary()->Find(parentGuid);
					}
					if(parent) {
						childIt->second->SetParent(parent, j.value("inheritScale", true));
					}
				}
			}

			SceneContext* ctx_ = nullptr;
			std::vector<Guid> rootGuids_;
			std::vector<nlohmann::json> snapshots_;
			SceneObjectEditCommandCallbacks callbacks_;
			std::string name_;
		};
	} // namespace

	std::unique_ptr<ICommand> CreateDeleteSceneObjectsCommand(
		SceneContext* ctx,
		std::vector<std::shared_ptr<SceneObject>> targets,
		SceneObjectEditCommandCallbacks callbacks,
		std::string name) {
		return std::make_unique<DeleteSceneObjectsCommand>(
			ctx,
			std::move(targets),
			std::move(callbacks),
			std::move(name));
	}

	DuplicateSceneObjectsCommand::DuplicateSceneObjectsCommand(
		SceneContext* ctx,
		std::vector<std::shared_ptr<SceneObject>> sources,
		SceneObjectEditCommandCallbacks callbacks)
		: BaseLevelEditorCommand("Duplicate Scene Objects"),
		  ctx_(ctx),
		  callbacks_(std::move(callbacks)) {
		sourceGuids_.reserve(sources.size());
		for(const auto& source : sources) {
			if(source) {
				sourceGuids_.push_back(source->GetGuid());
			}
		}
	}

	void DuplicateSceneObjectsCommand::Execute() {
		result_ = CreateDuplicates();
		if(callbacks_.selectObjects) callbacks_.selectObjects(result_.selectedRoots);
		if(callbacks_.refreshHierarchy) callbacks_.refreshHierarchy();
	}

	void DuplicateSceneObjectsCommand::Undo() {
		if(!ctx_ || !ctx_->GetObjectLibrary()) return;
		for(const auto& guid : result_.rootGuids) {
			auto object = ctx_->GetObjectLibrary()->Find(guid);
			if(object) {
				ctx_->RemoveObject(object);
			}
		}
		result_ = {};
		if(callbacks_.clearSelection) callbacks_.clearSelection();
		if(callbacks_.refreshHierarchy) callbacks_.refreshHierarchy();
	}

	void DuplicateSceneObjectsCommand::Redo() {
		Execute();
	}

	const std::vector<std::shared_ptr<SceneObject>>& DuplicateSceneObjectsCommand::GetCreatedRoots() const {
		return result_.selectedRoots;
	}

	SceneObjectDuplicateResult DuplicateSceneObjectsCommand::CreateDuplicates() {
		if(!ctx_) return {};

		if(auto* lib = ctx_->GetObjectLibrary()) {
			std::vector<std::shared_ptr<SceneObject>> sources;
			sources.reserve(sourceGuids_.size());
			for(const auto& guid : sourceGuids_) {
				auto source = lib->Find(guid);
				if(source) {
					sources.push_back(source);
				}
			}
			return SceneObjectDuplicator::Duplicate(ctx_, sources);
		}
		return {};
	}

} // namespace CalyxEngine
