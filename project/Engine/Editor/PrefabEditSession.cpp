#include "PrefabEditSession.h"

#include <Data/Engine/Prefab/Serializer/PrefabSerializer.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Assets/System/AssetType.h>
#include <Engine/Editor/Prefab/PrefabEditContextUtils.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Camera/3d/DebugCamera.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Lighting/LightLibrary.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/LightObject/DirectionalLight.h>
#include <Engine/Objects/LightObject/PointLight.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/System/SceneManager.h>

#include <filesystem>
#include <unordered_map>

namespace {
	void CollectPrefabInstanceGuidMap(
		const std::shared_ptr<SceneObject>& object,
		std::unordered_map<Guid, Guid>& sourceToInstanceGuid) {
		if(!object) return;

		const Guid& sourceGuid = object->GetPrefabSourceGuid();
		if(sourceGuid.isValid() && object->GetGuid().isValid()) {
			sourceToInstanceGuid[sourceGuid] = object->GetGuid();
		}

		for(const auto& child : object->GetChildren()) {
			CollectPrefabInstanceGuidMap(child, sourceToInstanceGuid);
		}
	}
}

namespace CalyxEngine {

	void PrefabEditSession::Ensure() {
		if(context_) return;

		SceneContext* previous = SceneContext::Current();

		context_ = std::make_unique<SceneContext>();
		context_->Initialize(false);
		context_->SetSceneName("PrefabEdit");

		if(auto* debugCamera = context_->GetCameraMgr()->GetDebug()) {
			debugCamera->SetTransient(true);
			debugCamera->GetWorldTransform().translation = {0.0f, 4.0f, -10.0f};
			debugCamera->GetWorldTransform().Update();
		}
		if(auto* mainCamera = context_->GetCameraMgr()->GetMain3d()) {
			mainCamera->SetTransient(true);
		}
		auto previewDirectionalLight = context_->Instantiate<DirectionalLight>("PrefabPreviewDirectionalLight");
		if(previewDirectionalLight) {
			previewDirectionalLight->SetTransient(true);
			previewDirectionalLight->SetEnableRaycast(false);
			context_->GetLightLibrary()->SetDirectionalLight(previewDirectionalLight);
		}
		auto previewPointLight = context_->Instantiate<PointLight>("PrefabPreviewPointLight");
		if(previewPointLight) {
			previewPointLight->SetTransient(true);
			previewPointLight->SetEnableRaycast(false);
			previewPointLight->GetWorldTransform().translation = {0.0f, 4.0f, -4.0f};
			previewPointLight->GetWorldTransform().Update();
			context_->GetLightLibrary()->SetPointLight(previewPointLight);
		}

		context_->AddOnObjectAddedListener([this](SceneObject*) {
			dirty_ = true;
		});
		context_->AddOnObjectRemovedListener([this](SceneObject*) {
			dirty_ = true;
		});

		if(previous) {
			previous->MakeCurrent();
		}
	}

	void PrefabEditSession::Reset() {
		context_.reset();
		path_.clear();
		dirty_ = false;
	}

	std::shared_ptr<SceneObject> PrefabEditSession::New(
		const std::string& rootTypeName,
		SceneManager* sceneManager) {
		Reset();
		Ensure();

		if(sceneManager) sceneManager->SetEditorPreviewContext(context_.get());
		if(!context_) return nullptr;

		context_->MakeCurrent();
		std::shared_ptr<SceneObject> root;
		if(!rootTypeName.empty()) {
			root = SceneObjectRegistry::Get().Create(rootTypeName);
			context_->AddObject(root);
		} else {
			root = context_->Instantiate<SceneObject>();
		}
		if(root) {
			root->SetName("NewPrefab", root->GetObjectType());
			root->SetEnableRaycast(true);
			root->Initialize();
		}
		return root;
	}

	std::shared_ptr<SceneObject> PrefabEditSession::Open(
		const std::string& path,
		SceneManager* sceneManager) {
		Reset();
		path_ = path;
		Ensure();
		if(!context_) return nullptr;

		context_->MakeCurrent();

		auto objects = PrefabSerializer::Load(path, PrefabSerializer::LoadOptions{true, Guid::Empty()});
		for(auto& object : objects) {
			if(object) {
				object->SetEnableRaycast(true);
				context_->AddObject(object);
			}
		}
		MarkUtilityObjects();
		NormalizeRoots();

		context_->SetSceneName(std::filesystem::path(path).stem().string());
		if(sceneManager) sceneManager->SetEditorPreviewContext(context_.get());
		context_->MakeCurrent();

		std::shared_ptr<SceneObject> selectedRoot;
		auto roots = GetRoots();
		if(!roots.empty()) {
			selectedRoot = context_->FindSharedObject(roots.front());
		}
		dirty_ = false;
		return selectedRoot;
	}

	void PrefabEditSession::Update(float dt) {
		if(!context_) return;

		SceneContext* previous = SceneContext::Current();
		context_->MakeCurrent();
		NormalizeRoots();
		context_->Update(dt, dt, false);

		if(previous && previous != context_.get()) {
			previous->MakeCurrent();
		}
	}

	std::vector<SceneObject*> PrefabEditSession::GetRoots() const {
		if(!context_) return {};
		return PrefabEditContextUtils::GetSerializableRoots(*context_);
	}

	void PrefabEditSession::MarkUtilityObjects() {
		if(context_) PrefabEditContextUtils::MarkEditorUtilityObjects(*context_);
	}

	void PrefabEditSession::NormalizeRoots() {
		if(context_) PrefabEditContextUtils::NormalizeRoots(*context_);
	}

	bool PrefabEditSession::Save(SceneManager* sceneManager) {
		if(!context_ || path_.empty()) return false;
		return SaveAs(path_, sceneManager);
	}

	bool PrefabEditSession::SaveAs(const std::string& path, SceneManager* sceneManager) {
		if(!context_) return false;

		const auto roots = GetRoots();
		if(roots.empty()) return false;
		NormalizeRoots();

		const std::filesystem::path savePath(path);
		if(savePath.has_parent_path()) {
			std::error_code ec;
			std::filesystem::create_directories(savePath.parent_path(), ec);
		}

		if(!PrefabSerializer::Save(roots, path, PrefabSerializer::SaveOptions{true})) {
			return false;
		}

		path_ = path;
		dirty_ = false;
		if(auto* db = AssetDatabase::GetInstance()) {
			const Guid prefabGuid = db->RegisterOrUpdate(path, AssetType::Prefab);
			db->Scan();
			if(prefabGuid.isValid()) {
				SyncInstancesInCurrentScene(prefabGuid, path, sceneManager);
			}
		}
		return true;
	}

	bool PrefabEditSession::ApplyOverridesFromInstance(
		const std::shared_ptr<SceneObject>& object,
		SceneManager* sceneManager) {
		if(!object || !object->IsPrefabInstanceObject()) return false;

		const Guid prefabGuid = object->GetPrefabAssetGuid();
		auto prefabRoot = object;
		while(auto parent = prefabRoot->GetParent()) {
			if(parent->GetPrefabAssetGuid() != prefabGuid) break;
			prefabRoot = parent;
		}

		auto* db = AssetDatabase::GetInstance();
		if(!db) return false;
		const AssetRecord* record = db->Get(prefabGuid);
		if(!record || record->type != AssetType::Prefab) return false;

		const std::string path = record->sourcePath.string();
		PrefabSerializer::SaveOptions saveOptions;
		saveOptions.resetRootTransform = true;
		saveOptions.usePrefabSourceGuids = true;
		if(!PrefabSerializer::Save({prefabRoot.get()}, path, saveOptions)) {
			return false;
		}

		const Guid registeredGuid = db->RegisterOrUpdate(path, AssetType::Prefab);
		db->Scan();
		SyncInstancesInCurrentScene(registeredGuid.isValid() ? registeredGuid : prefabGuid, path, sceneManager);
		return true;
	}

	void PrefabEditSession::SyncInstancesInCurrentScene(
		const Guid& prefabAssetGuid,
		const std::string& prefabPath,
		SceneManager* sceneManager) {
		if(!prefabAssetGuid.isValid() || prefabPath.empty() || !sceneManager) return;

		SceneContext* sceneCtx = sceneManager->GetCurrentSceneContext();
		if(!sceneCtx || sceneCtx == context_.get()) return;
		auto* sceneLib = sceneCtx->GetObjectLibrary();
		if(!sceneLib) return;

		std::vector<std::shared_ptr<SceneObject>> instanceRoots;
		for(auto& object : sceneLib->GetAllObjectsShared()) {
			if(!object || object->GetPrefabAssetGuid() != prefabAssetGuid) continue;

			auto parent = object->GetParent();
			if(parent && parent->GetPrefabAssetGuid() == prefabAssetGuid) continue;
			instanceRoots.push_back(object);
		}

		if(instanceRoots.empty()) return;

		SceneContext* previous = SceneContext::Current();
		sceneCtx->MakeCurrent();

		for(auto& oldRoot : instanceRoots) {
			if(!oldRoot || !sceneLib->Contains(oldRoot)) continue;

			const Guid oldRootGuid = oldRoot->GetGuid();
			const Guid oldSourceGuid = oldRoot->GetPrefabSourceGuid();
			const std::string oldName = oldRoot->GetName();
			const WorldTransform oldTransform = oldRoot->GetWorldTransform();
			auto oldParent = oldRoot->GetParent();
			const bool inheritScale = oldRoot->GetWorldTransform().inheritScale;
			std::unordered_map<Guid, Guid> sourceToInstanceGuid;
			CollectPrefabInstanceGuidMap(oldRoot, sourceToInstanceGuid);

			auto loadedObjects = PrefabSerializer::Load(
				prefabPath,
				PrefabSerializer::LoadOptions{false, prefabAssetGuid});

			std::shared_ptr<SceneObject> newRoot;
			for(auto& candidate : loadedObjects) {
				if(!candidate) continue;
				if(candidate->GetPrefabSourceGuid() == oldSourceGuid) {
					newRoot = candidate;
					break;
				}
			}
			if(!newRoot) {
				for(auto& candidate : loadedObjects) {
					if(candidate && !candidate->GetParent()) {
						newRoot = candidate;
						break;
					}
				}
			}
			if(!newRoot) continue;

			sceneCtx->RemoveObject(oldRoot);

			std::unordered_map<Guid, Guid> loadedToInstanceGuid;
			for(auto& object : loadedObjects) {
				if(!object) continue;

				const Guid& sourceGuid = object->GetPrefabSourceGuid();
				if(auto it = sourceToInstanceGuid.find(sourceGuid); it != sourceToInstanceGuid.end()) {
					loadedToInstanceGuid[object->GetGuid()] = it->second;
					object->SetGuid(it->second);
				}
			}
			for(auto& object : loadedObjects) {
				if(object && !loadedToInstanceGuid.empty()) {
					object->RemapSceneObjectReferences(loadedToInstanceGuid);
				}
			}

			newRoot->SetGuid(oldRootGuid);
			newRoot->SetName(oldName, newRoot->GetObjectType());
			newRoot->GetWorldTransform() = oldTransform;
			newRoot->GetWorldTransform().parent = nullptr;
			newRoot->GetWorldTransform().Update();

			for(auto& object : loadedObjects) {
				if(object) {
					sceneCtx->AddObject(object);
				}
			}
			if(oldParent && sceneLib->Contains(oldParent)) {
				newRoot->SetParent(oldParent, inheritScale);
			}
		}

		if(previous && previous != sceneCtx) {
			previous->MakeCurrent();
		}
	}

} // namespace CalyxEngine
