#include "AssetPreviewManager.h"

#include <Data/Engine/Prefab/Serializer/PrefabSerializer.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Assets/Manager/AssetManager.h>
#include <Engine/Assets/Model/Model.h>
#include <Engine/Assets/Model/ModelManager.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Foundation/Debug/CxAssert.h>
#include <Engine/Foundation/Math/MathUtil.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Graphics/Camera/3d/DebugCamera.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/GpuResource/DxGpuResource.h>
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/Event/BaseEventObject.h>
#include <Engine/Renderer/Model/ModelRenderer.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <array>
#include <cmath>
#include <system_error>

namespace CalyxEngine {
	namespace {
		void CollectObjectsRecursive(SceneObject* object, std::vector<SceneObject*>& out) {
			if(!object) return;
			out.push_back(object);
			for(const auto& child : object->GetChildren()) {
				CollectObjectsRecursive(child.get(), out);
			}
		}

		AABB MergeAabb(const AABB& a, const AABB& b) {
			return AABB{
				{
					(std::min)(a.min_.x, b.min_.x),
					(std::min)(a.min_.y, b.min_.y),
					(std::min)(a.min_.z, b.min_.z),
				},
				{
					(std::max)(a.max_.x, b.max_.x),
					(std::max)(a.max_.y, b.max_.y),
					(std::max)(a.max_.z, b.max_.z),
				}};
		}

		WorldTransform MakePreviewTransform() {
			WorldTransform transform;
			transform.scale		  = {1.0f, 1.0f, 1.0f};
			transform.rotation	  = CalyxEngine::Quaternion::MakeIdentity();
			transform.translation = CalyxEngine::Vector3::Zero();
			transform.matrix.world = CalyxEngine::MakeAffineMatrix(
				transform.scale,
				transform.rotation,
				transform.translation);
			transform.matrix.WorldInverseTranspose =
				CalyxEngine::Matrix4x4::Transpose(CalyxEngine::Matrix4x4::Inverse(transform.matrix.world));
			return transform;
		}
	}

	AssetPreviewManager AssetPreviewManager::instance_;

	AssetPreviewManager* AssetPreviewManager::GetInstance() {
		return &instance_;
	}

	bool AssetPreviewManager::IsPreviewSupported(AssetType type) const {
		return type == AssetType::Model;
	}

	void AssetPreviewManager::Request(const AssetRecord& record) {
		if(!record.guid.isValid() || !IsPreviewSupported(record.type)) return;

		auto& entry = entries_[record.guid];
		if(entry.state == State::Ready && entry.lastWrite == record.lastWrite && entry.type == record.type) return;
		if(entry.state == State::Queued && entry.lastWrite == record.lastWrite && entry.type == record.type) return;
		if(entry.state == State::Failed && entry.lastWrite == record.lastWrite && entry.type == record.type) return;

		entry.state		= State::Queued;
		entry.texture	= nullptr;
		entry.lastWrite = record.lastWrite;
		entry.type		= record.type;
		entry.modelLoadRequested = false;
		queue_.push_back(record.guid);
	}

	void AssetPreviewManager::Invalidate(const Guid& guid) {
		if(!guid.isValid()) return;
		auto it = entries_.find(guid);
		if(it == entries_.end()) return;
		RetireEntryResources(it->second);
		entries_.erase(it);
	}

	void AssetPreviewManager::InvalidateAll() {
		for(auto& [guid, entry] : entries_) {
			(void)guid;
			RetireEntryResources(entry);
		}
		entries_.clear();
		queue_.clear();
	}

	void AssetPreviewManager::Shutdown() {
		queue_.clear();
		entries_.clear();
		retainedFrameObjects_.clear();
		retainedFrameModels_.clear();
		retiredFrameResources_.clear();
		modelRenderer_.reset();
		previewContext_.reset();
	}

	AssetPreviewManager::PreviewResult AssetPreviewManager::GetPreview(const AssetRecord& record, ImTextureID fallback) {
		if(!IsPreviewSupported(record.type)) {
			return PreviewResult{record.previewTex ? record.previewTex : fallback, record.previewTex != nullptr};
		}

		Request(record);

		auto it = entries_.find(record.guid);
		if(it != entries_.end() && it->second.state == State::Ready && it->second.texture) {
			return PreviewResult{it->second.texture, true};
		}

		return PreviewResult{fallback, false};
	}

	void AssetPreviewManager::ProcessQueue(int maxItemsPerFrame) {
		if(maxItemsPerFrame <= 0) return;

		auto* db = AssetDatabase::GetInstance();
		if(!db) return;

		int processed = 0;
		while(processed < maxItemsPerFrame && !queue_.empty()) {
			const Guid guid = queue_.front();
			queue_.pop_front();

			auto it = entries_.find(guid);
			if(it == entries_.end() || it->second.state != State::Queued) continue;

			const AssetRecord* record = db->Get(guid);
			if(!record || !IsPreviewSupported(record->type)) {
				entries_.erase(guid);
				continue;
			}

			if(record->lastWrite != it->second.lastWrite || record->type != it->second.type) {
				Request(*record);
				continue;
			}

			it->second.state = TryGeneratePreview(*record, it->second) ? State::Ready : State::Queued;
			++processed;
		}
	}

	void AssetPreviewManager::ProcessRenderQueue(ID3D12GraphicsCommandList* cmdList,
												 PipelineService*			 pso,
												 IRenderTarget*				 renderTarget,
												 int						 maxItemsPerFrame) {
		if(!cmdList || !pso || !renderTarget || maxItemsPerFrame <= 0) return;

		auto* db = AssetDatabase::GetInstance();
		if(!db) return;

		int processed = 0;
		while(processed < maxItemsPerFrame && !queue_.empty()) {
			const Guid guid = queue_.front();
			queue_.pop_front();

			auto it = entries_.find(guid);
			if(it == entries_.end() || it->second.state != State::Queued) continue;

			const AssetRecord* record = db->Get(guid);
			if(!record || !IsPreviewSupported(record->type)) {
				entries_.erase(guid);
				continue;
			}

			if(record->lastWrite != it->second.lastWrite || record->type != it->second.type) {
				Request(*record);
				continue;
			}

			if(record->type == AssetType::Model) {
				auto* assetManager = AssetManager::GetInstance();
				auto* modelManager = assetManager ? assetManager->GetModelManager() : nullptr;
				const std::string modelName = record->sourcePath.filename().string();
				if(modelManager && !modelManager->IsModelLoaded(modelName)) {
					if(!it->second.modelLoadRequested) {
						modelManager->LoadModel(modelName);
						it->second.modelLoadRequested = true;
					}
					modelManager->ProcessLoadingTasks();
					if(!modelManager->IsModelLoaded(modelName)) {
						it->second.state = State::Queued;
						queue_.push_back(guid);
						++processed;
						continue;
					}
				}
			}

			if(TryGeneratePreview(*record, it->second) ||
			   TryRenderModelPreview(*record, it->second, cmdList, pso, renderTarget)) {
				it->second.state = State::Ready;
			} else {
				it->second.state = State::Failed;
			}
			++processed;
		}
	}

	void AssetPreviewManager::ReleaseFrameResources() {
		retainedFrameObjects_.clear();
		retainedFrameModels_.clear();
		retiredFrameResources_.clear();
	}

	bool AssetPreviewManager::TryGeneratePreview(const AssetRecord& record, Entry& entry) {
		if(TryLoadSidecarPreview(record, entry)) return true;

		// Runtime render capture is intentionally centralized here:
		// render into one reusable OffscreenRenderTarget, copy it into a
		// per-asset texture or atlas slot, then store its SRV in entry.texture.
		return false;
	}

	bool AssetPreviewManager::TryRenderPreview(const AssetRecord& record,
											   Entry&			 entry,
											   ID3D12GraphicsCommandList* cmdList,
											   PipelineService*			  pso,
											   IRenderTarget*			  renderTarget) {
		if(!EnsurePreviewContext()) return false;

		SceneContext* previous = SceneContext::Current();
		previewContext_->MakeCurrent();

		std::vector<std::shared_ptr<SceneObject>> objects;
		try {
			objects = CreatePreviewObjects(record);
		} catch(const std::exception& e) {
			const std::string message = "Asset preview failed: " + record.sourcePath.filename().string() + " / " + e.what();
			CX_WARN(message.c_str());
			if(previous) previous->MakeCurrent();
			return false;
		} catch(...) {
			const std::string message = "Asset preview failed: " + record.sourcePath.filename().string();
			CX_WARN(message.c_str());
			if(previous) previous->MakeCurrent();
			return false;
		}
		if(objects.empty()) {
			if(previous) previous->MakeCurrent();
			return false;
		}

		ConfigureCamera(objects);
		previewContext_->GetCameraMgr()->TransferToGPU();
		previewContext_->GetLightLibrary()->CyncGpu();

		renderTarget->Clear(cmdList);
		renderTarget->SetRenderTarget(cmdList);

		modelRenderer_->BeginFrame();
		for(const auto& root : objects) {
			std::vector<SceneObject*> flattened;
			CollectObjectsRecursive(root.get(), flattened);
			for(SceneObject* object : flattened) {
				if(object) object->AlwaysUpdate(0.0f);
				RegisterPreviewObject(object);
			}
		}

		if(auto* camera = dynamic_cast<Camera3d*>(CameraManager::GetActive())) {
			modelRenderer_->PreCullAndBatch(camera, false);
		} else {
			modelRenderer_->BuildAllVisibleBatches();
		}

		modelRenderer_->DrawAll(cmdList,
								GraphicsGroup::GetInstance()->GetDevice().Get(),
								renderTarget,
								pso,
								previewContext_->GetLightLibrary(),
								nullptr);

		const bool copied = CopyRenderTargetToCache(cmdList, renderTarget, entry);
		retainedFrameObjects_.insert(retainedFrameObjects_.end(), objects.begin(), objects.end());
		if(previous) previous->MakeCurrent();
		return copied;
	}

	bool AssetPreviewManager::TryRenderModelPreview(const AssetRecord& record,
													Entry&			  entry,
													ID3D12GraphicsCommandList* cmdList,
													PipelineService*		   pso,
													IRenderTarget*			   renderTarget) {
		if(record.type != AssetType::Model) return false;
		if(!EnsurePreviewContext()) return false;

		const std::string modelName = record.sourcePath.filename().string();
		auto			  model		= std::make_shared<Model>(modelName);
		model->Update(0.0f);
		if(!model->GetModelData()) return false;

		const AABB& bounds = model->GetModelData()->localAABB;
		Vector3 center = (bounds.min_ + bounds.max_) * 0.5f;
		const Vector3 extents = (bounds.max_ - bounds.min_) * 0.5f;
		const float radius = (std::max)(0.75f, extents.Length());

		SceneContext* previous = SceneContext::Current();
		previewContext_->MakeCurrent();

		CameraManager::SetTypeStatic(CameraType::Debug);
		CameraManager::SetViewportSizeStatic(ViewportType::VIEWPORT_DEBUG, {256.0f, 256.0f});
		previewContext_->GetCameraMgr()->SetAspectRatio(1.0f, 1.0f);
		if(auto* debugCamera = CameraManager::GetDebug()) {
			DebugCamera::State state = debugCamera->CaptureState();
			state.target = center;
			state.distance = radius * 2.8f;
			state.orbitAngle = {0.65f, 0.35f};
			debugCamera->ApplyState(state);
			debugCamera->AlwaysUpdate(0.0f);
		}
		previewContext_->GetCameraMgr()->TransferToGPU();
		previewContext_->GetLightLibrary()->CyncGpu();

		renderTarget->Clear(cmdList);
		renderTarget->SetRenderTarget(cmdList);

		WorldTransform transform = MakePreviewTransform();
		modelRenderer_->BeginFrame();
		modelRenderer_->RegisterStatic(model.get(), transform, BillboardMode::None, nullptr);

		if(auto* camera = dynamic_cast<Camera3d*>(CameraManager::GetActive())) {
			modelRenderer_->PreCullAndBatch(camera, false);
		} else {
			modelRenderer_->BuildAllVisibleBatches();
		}

		modelRenderer_->DrawAll(cmdList,
								GraphicsGroup::GetInstance()->GetDevice().Get(),
								renderTarget,
								pso,
								previewContext_->GetLightLibrary(),
								nullptr);

		const bool copied = CopyRenderTargetToCache(cmdList, renderTarget, entry);
		retainedFrameModels_.push_back(std::move(model));
		if(previous) previous->MakeCurrent();
		return copied;
	}

	bool AssetPreviewManager::EnsurePreviewContext() {
		if(!previewContext_) {
			previewContext_ = std::make_unique<SceneContext>();
			previewContext_->Initialize(true);
			if(auto* debugCamera = CameraManager::GetDebug()) {
				debugCamera->SetInputEnabled(false);
			}
		}
		if(!modelRenderer_) {
			modelRenderer_ = std::make_unique<ModelRenderer>();
		}
		return previewContext_ && modelRenderer_;
	}

	std::vector<std::shared_ptr<SceneObject>> AssetPreviewManager::CreatePreviewObjects(const AssetRecord& record) {
		if(record.type == AssetType::Model) {
			auto* assetManager = AssetManager::GetInstance();
			if(assetManager && assetManager->GetModelManager()) {
				const std::string modelName = record.sourcePath.filename().string();
				assetManager->GetModelManager()->LoadModel(modelName);
				assetManager->GetModelManager()->ProcessLoadingTasks();

				auto object = std::make_shared<BaseGameObject>(modelName, record.sourcePath.stem().string());
				object->Initialize();
				object->AlwaysUpdate(0.0f);
				return {object};
			}
			return {};
		}

		if(record.type == AssetType::Prefab) {
			PrefabSerializer::LoadOptions options;
			options.preserveGuids	= false;
			options.prefabAssetGuid	= record.guid;
			options.skipUnknownTypes = true;
			return PrefabSerializer::Load(record.sourcePath.string(),
										  options);
		}

		return {};
	}

	void AssetPreviewManager::RegisterPreviewObject(SceneObject* object) {
		if(!object) return;

		if(auto* go = dynamic_cast<BaseGameObject*>(object)) {
			switch(go->GetModelType()) {
			case ObjectModelType::ModelType_Static:
				if(auto* model = go->GetStaticModel()) {
					modelRenderer_->RegisterStatic(model, go->GetWorldTransform(), go->GetBillboardMode(), go);
				}
				break;
			case ObjectModelType::ModelType_Animation:
				if(auto* model = go->AnimationModel()) {
					modelRenderer_->RegisterSkinned(model, go->GetWorldTransform(), go);
				}
				break;
			default:
				break;
			}
			return;
		}

		if(auto* eventObject = dynamic_cast<BaseEventObject*>(object)) {
			if(auto* model = eventObject->GetModel()) {
				modelRenderer_->RegisterStatic(model, eventObject->GetWorldTransform(), BillboardMode::None, eventObject);
			}
		}
	}

	void AssetPreviewManager::ConfigureCamera(const std::vector<std::shared_ptr<SceneObject>>& roots) {
		AABB bounds{};
		bool hasBounds = false;

		for(const auto& root : roots) {
			std::vector<SceneObject*> flattened;
			CollectObjectsRecursive(root.get(), flattened);
			for(SceneObject* object : flattened) {
				if(auto* go = dynamic_cast<BaseGameObject*>(object)) {
					const AABB objectBounds = go->GetWorldAABB();
					bounds = hasBounds ? MergeAabb(bounds, objectBounds) : objectBounds;
					hasBounds = true;
				}
			}
		}

		Vector3 center{0.0f, 0.5f, 0.0f};
		float	radius = 1.5f;
		if(hasBounds) {
			center = (bounds.min_ + bounds.max_) * 0.5f;
			const Vector3 extents = (bounds.max_ - bounds.min_) * 0.5f;
			radius = (std::max)(0.75f, extents.Length());
		}

		CameraManager::SetTypeStatic(CameraType::Debug);
		CameraManager::SetViewportSizeStatic(ViewportType::VIEWPORT_DEBUG, {256.0f, 256.0f});
		previewContext_->GetCameraMgr()->SetAspectRatio(1.0f, 1.0f);

		if(auto* debugCamera = CameraManager::GetDebug()) {
			DebugCamera::State state = debugCamera->CaptureState();
			state.target = center;
			state.distance = radius * 2.8f;
			state.orbitAngle = {0.65f, 0.35f};
			debugCamera->ApplyState(state);
			debugCamera->AlwaysUpdate(0.0f);
		}
	}

	bool AssetPreviewManager::CopyRenderTargetToCache(ID3D12GraphicsCommandList* cmdList,
													  IRenderTarget*				 renderTarget,
													  Entry&					 entry) {
		auto* source = renderTarget->GetResource();
		if(!source || !source->Get()) return false;

		auto* device = GraphicsGroup::GetInstance()->GetDevice().Get();
		if(!device) return false;

		const auto desc = source->Get()->GetDesc();
		RetireEntryResources(entry);
		entry.cacheResource = std::make_unique<DxGpuResource>();
		entry.cacheResource->InitializeAsRenderTarget(device,
													  static_cast<uint32_t>(desc.Width),
													  desc.Height,
													  desc.Format,
													  L"AssetPreviewCache");
		entry.cacheResource->CreateSRV(device);

		renderTarget->TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		entry.cacheResource->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
		cmdList->CopyResource(entry.cacheResource->Get(), source->Get());
		entry.cacheResource->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		renderTarget->TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		entry.texture = (ImTextureID)entry.cacheResource->GetSRVGpuHandle().ptr;
		return entry.texture != nullptr;
	}

	void AssetPreviewManager::RetireEntryResources(Entry& entry) {
		if(entry.cacheResource) {
			retiredFrameResources_.push_back(std::move(entry.cacheResource));
		}
		entry.texture = nullptr;
	}

	bool AssetPreviewManager::TryLoadSidecarPreview(const AssetRecord& record, Entry& entry) {
		auto previewPath = FindSidecarPreviewPath(record);
		if(!previewPath.has_value()) return false;

		auto* db = AssetDatabase::GetInstance();
		auto* tm = AssetManager::GetInstance()->GetTextureManager();
		if(!db || !tm) return false;

		std::error_code ec;
		const auto rel = std::filesystem::relative(*previewPath, db->GetRoot(), ec);
		if(ec) return false;

		const auto handle = tm->LoadTexture(rel.generic_string());
		if(handle.ptr == 0) return false;

		RetireEntryResources(entry);
		entry.texture = (ImTextureID)handle.ptr;
		return true;
	}

	std::optional<std::filesystem::path> AssetPreviewManager::FindSidecarPreviewPath(const AssetRecord& record) const {
		const std::array<const char*, 4> suffixes = {
			".preview.dds",
			".preview.png",
			"_preview.dds",
			"_preview.png",
		};

		for(const char* suffix : suffixes) {
			std::filesystem::path path = record.sourcePath;
			if(suffix[0] == '_') {
				path.replace_filename(record.sourcePath.stem().string() + suffix);
			} else {
				path += suffix;
			}

			std::error_code ec;
			if(std::filesystem::exists(path, ec) && !ec) return path;
		}

		return std::nullopt;
	}

} // namespace CalyxEngine
