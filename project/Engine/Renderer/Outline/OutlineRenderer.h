#pragma once

#include <Engine\Foundation\Math\Vector2.h>
#include <Engine\Foundation\Math\Vector4.h>
#include <Engine\Graphics\Descriptor\DescriptorAllocator.h>
#include <Engine\Graphics\GpuResource\DxGpuResource.h>
#include <Engine\Renderer\Model\ModelRenderer.h>

#include <d3d12.h>
#include <vector>

class Camera3d;
class IRenderTarget;
class PipelineService;
class SceneObject;

class OutlineRenderer {
public:
	void Render(ID3D12GraphicsCommandList* cmdList,
				ID3D12Device* device,
				IRenderTarget* rt,
				PipelineService* psoService,
				const Camera3d* camera,
				const ModelRenderer& modelRenderer);
	void RenderSelectionHighlight(ID3D12GraphicsCommandList* cmdList,
								  ID3D12Device* device,
								  IRenderTarget* rt,
								  PipelineService* psoService,
								  const Camera3d* camera,
								  const ModelRenderer& modelRenderer,
								  const SceneObject* selected);
	void RenderSelectionHighlight(ID3D12GraphicsCommandList* cmdList,
								  ID3D12Device* device,
								  IRenderTarget* rt,
								  PipelineService* psoService,
								  const Camera3d* camera,
								  const ModelRenderer& modelRenderer,
								  const std::vector<SceneObject*>& selectedObjects);

private:
	struct CompositeConstants {
		CalyxEngine::Vector4 color = {0.02f, 0.02f, 0.025f, 1.0f};
		CalyxEngine::Vector2 texelSize = {1.0f, 1.0f};
		float depthThreshold = 0.0025f;
		float normalThreshold = 0.18f;
		float depthScale = 90.0f;
		float normalScale = 1.8f;
		float thickness = 2.0f;
		float insideMaskOnly = 0.0f;
		CalyxEngine::Vector4 padding = {0.0f, 0.0f, 0.0f, 0.0f};
	};

	struct StaticBatch {
		BaseModel*						   model = nullptr;
		std::vector<WorldTransform>		   transforms;
		std::vector<GpuBillboardParams>	   billboards;
	};

	void EnsureResources(ID3D12Device* device, const D3D12_VIEWPORT& viewport);
	bool RenderNormalBuffer(ID3D12GraphicsCommandList* cmdList,
							ID3D12Device* device,
							IRenderTarget* rt,
							PipelineService* psoService,
							const Camera3d* camera,
							const ModelRenderer& modelRenderer,
							const SceneObject* targetOwner = nullptr);
	void Composite(ID3D12GraphicsCommandList* cmdList,
				   IRenderTarget* rt,
				   PipelineService* psoService);

	DxGpuResource normalResource_;
	DxGpuResource compositeResource_;
	DxGpuResource selectionDepthResource_;
	DescriptorHandle normalRtv_{};
	DescriptorHandle compositeRtv_{};
	DescriptorHandle selectionDepthDsv_{};
	CompositeConstants compositeConstants_{};
	uint32_t width_ = 0;
	uint32_t height_ = 0;
};
