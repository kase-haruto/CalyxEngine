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

namespace CalyxEngine {

	void PrefabEditSession::Ensure() {
		if(context_) return;

		// Prefab専用Contextの初期化中にCurrentが切り替わるため、呼び出し元の編集Contextを退避する。
		SceneContext* previous = SceneContext::Current();

		// Runtime SceneへPreview用CameraやLightを混入させないよう、独立Contextを作成する。
		context_ = std::make_unique<SceneContext>();
		context_->Initialize(false);
		context_->SetSceneName("PrefabEdit");

		// Preview補助ObjectはTransientにし、Prefab保存対象から確実に除外する。
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

		// Context内の構造変更を監視し、未保存状態をUIへ反映する。
		context_->AddOnObjectAddedListener([this](SceneObject*) {
			dirty_ = true;
		});
		context_->AddOnObjectRemovedListener([this](SceneObject*) {
			dirty_ = true;
		});

		// 初期化前のCurrent Contextを復元し、他Editor処理の参照先を変更しない。
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
		// 以前のPrefab編集データを破棄して、空のPreview Contextから新規Rootを構築する。
		Reset();
		Ensure();

		if(sceneManager) sceneManager->SetEditorPreviewContext(context_.get());
		if(!context_) return nullptr;

		// Registry生成やInitializeがPrefab用Manager群を参照するよう、生成中だけ専用ContextをCurrentにする。
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

		// Editor補助Objectを含めず、保存時のSource GUIDを保持した編集用Hierarchyとして読み込む。
		auto objects = PrefabSerializer::Load(path, PrefabSerializer::LoadOptions{true, Guid::Empty()});
		for(auto& object : objects) {
			if(object) {
				object->SetEnableRaycast(true);
				context_->AddObject(object);
			}
		}
		// Serializerの結果へPreview属性を適用し、複数RootのTransformを編集原点へ正規化する。
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

		// Prefab ObjectのUpdateだけを専用Contextで実行し、終了後は元のSceneへ必ず戻す。
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

		// TransientなPreview Camera/Lightを除いたRootだけをPrefabデータとして抽出する。
		const auto roots = GetRoots();
		if(roots.empty()) return false;
		NormalizeRoots();

		// 初回保存先でもSerializerが失敗しないよう、親Directoryを先に用意する。
		const std::filesystem::path savePath(path);
		if(savePath.has_parent_path()) {
			std::error_code ec;
			std::filesystem::create_directories(savePath.parent_path(), ec);
		}

		if(!PrefabSerializer::Save(roots, path, PrefabSerializer::SaveOptions{true})) {
			return false;
		}

		// 保存成功後にAssetDatabaseを更新し、開いているScene内の同一Prefab Instanceへ反映する。
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

		// 子Objectが選択されていても、同一Prefab GUIDが連続する最上位Rootまで遡る。
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
		// Instance固有の配置はAssetへ書き戻さず、Source GUIDは既存Instance同期の照合用に保持する。
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

		// Prefab子要素を除外し、置換単位となるInstance Rootだけを収集する。
		std::vector<std::shared_ptr<SceneObject>> instanceRoots;
		for(auto& object : sceneLib->GetAllObjectsShared()) {
			if(!object || object->GetPrefabAssetGuid() != prefabAssetGuid) continue;

			auto parent = object->GetParent();
			if(parent && parent->GetPrefabAssetGuid() == prefabAssetGuid) continue;
			instanceRoots.push_back(object);
		}

		if(instanceRoots.empty()) return;

		// Deserialize時に正しいScene Manager群を参照させるため、同期対象SceneをCurrentへ切り替える。
		SceneContext* previous = SceneContext::Current();
		sceneCtx->MakeCurrent();

		for(auto& oldRoot : instanceRoots) {
			if(!oldRoot || !sceneLib->Contains(oldRoot)) continue;

			// Scene内参照と配置を壊さないよう、置換前Rootの識別情報とTransformを退避する。
			const Guid oldRootGuid = oldRoot->GetGuid();
			const Guid oldSourceGuid = oldRoot->GetPrefabSourceGuid();
			const std::string oldName = oldRoot->GetName();
			const WorldTransform oldTransform = oldRoot->GetWorldTransform();
			auto oldParent = oldRoot->GetParent();
			const bool inheritScale = oldRoot->GetWorldTransform().inheritScale;

			// 更新済みAssetから新Hierarchyを生成し、Source GUIDが一致するRootを優先して選ぶ。
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

			// 旧Hierarchyを除去してから新Hierarchyを登録し、GUID重複期間を作らない。
			sceneCtx->RemoveObject(oldRoot);

			// 外部参照・Scene上の名前・配置を維持しつつ、Prefab内部構造だけを最新版へ交換する。
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

		// 同期処理後は元のEditor/Runtime Contextへ戻し、後続処理の対象Sceneを維持する。
		if(previous && previous != sceneCtx) {
			previous->MakeCurrent();
		}
	}

} // namespace CalyxEngine
