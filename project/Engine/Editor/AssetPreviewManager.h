#pragma once

#include <Engine/Assets/System/AssetRecord.h>
#include <Engine/Assets/System/AssetType.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

#include <externals/imgui/imgui.h>

#include <deque>
#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class DxGpuResource;
class IRenderTarget;
class ModelRenderer;
class PipelineService;
class SceneContext;
class SceneObject;
struct ID3D12GraphicsCommandList;

namespace CalyxEngine {

	class AssetPreviewManager {
	public:
		struct PreviewResult {
			ImTextureID texture = nullptr;
			bool		ready	= false;
		};

		static AssetPreviewManager* GetInstance();

		void Request(const AssetRecord& record);
		void Invalidate(const Guid& guid);
		void InvalidateAll();
		void Shutdown();
		void ProcessQueue(int maxItemsPerFrame = 1);
		void ProcessRenderQueue(ID3D12GraphicsCommandList* cmdList,
								PipelineService*			  pso,
								IRenderTarget*				  renderTarget,
								int							  maxItemsPerFrame = 1);
		void ReleaseFrameResources();

		PreviewResult GetPreview(const AssetRecord& record, ImTextureID fallback);
		bool		  IsPreviewSupported(AssetType type) const;

	private:
		enum class State {
			None,
			Queued,
			Ready,
			Failed
		};

		struct Entry {
			State						   state	 = State::None;
			ImTextureID					   texture	 = nullptr;
			std::filesystem::file_time_type lastWrite = {};
			AssetType					   type		 = AssetType::Unknown;
			std::unique_ptr<DxGpuResource> cacheResource;
		};

		static AssetPreviewManager instance_;

		bool TryGeneratePreview(const AssetRecord& record, Entry& entry);
		bool TryRenderPreview(const AssetRecord& record,
							  Entry&			 entry,
							  ID3D12GraphicsCommandList* cmdList,
							  PipelineService*			  pso,
							  IRenderTarget*				  renderTarget);
		bool TryLoadSidecarPreview(const AssetRecord& record, Entry& entry);
		std::optional<std::filesystem::path> FindSidecarPreviewPath(const AssetRecord& record) const;
		bool								EnsurePreviewContext();
		std::vector<std::shared_ptr<SceneObject>> CreatePreviewObjects(const AssetRecord& record);
		void								RegisterPreviewObject(SceneObject* object);
		void								ConfigureCamera(const std::vector<std::shared_ptr<SceneObject>>& roots);
		bool								CopyRenderTargetToCache(ID3D12GraphicsCommandList* cmdList,
																	 IRenderTarget*				  renderTarget,
																	 Entry&						  entry);
		void								RetireEntryResources(Entry& entry);

		std::unordered_map<Guid, Entry> entries_;
		std::deque<Guid>				  queue_;
		std::unique_ptr<SceneContext>	  previewContext_;
		std::unique_ptr<ModelRenderer>	  modelRenderer_;
		std::vector<std::shared_ptr<SceneObject>> retainedFrameObjects_;
		std::vector<std::unique_ptr<DxGpuResource>> retiredFrameResources_;
	};

} // namespace CalyxEngine
