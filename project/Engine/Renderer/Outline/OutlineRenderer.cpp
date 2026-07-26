#include "OutlineRenderer.h"

#include <Engine\Assets\Animation\AnimationModel.h>
#include <Engine\Assets\Model\BaseModel.h>
#include <Engine\Graphics\Camera\3d\Camera3d.h>
#include <Engine\Graphics\Pipeline\Pso\PsoDetails.h>
#include <Engine\Graphics\RenderTarget\Interface\IRenderTarget.h>
#include <Engine\Objects\3D\Actor\SceneObject.h>
#include <Engine\PostProcess\FullscreenDrawer.h>

#include <algorithm>

namespace {
	constexpr DXGI_FORMAT kNormalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	constexpr float kDitherFadeFar = 8.0f;

	D3D12_RECT MakeRect(uint32_t width, uint32_t height) {
		return D3D12_RECT{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
	}

	float DistanceSqPointAabb(const CalyxEngine::Vector3& p, const AABB& aabb) {
		float dx = 0.0f;
		if(p.x < aabb.min_.x) dx = aabb.min_.x - p.x;
		else if(p.x > aabb.max_.x) dx = p.x - aabb.max_.x;

		float dy = 0.0f;
		if(p.y < aabb.min_.y) dy = aabb.min_.y - p.y;
		else if(p.y > aabb.max_.y) dy = p.y - aabb.max_.y;

		float dz = 0.0f;
		if(p.z < aabb.min_.z) dz = aabb.min_.z - p.z;
		else if(p.z > aabb.max_.z) dz = p.z - aabb.max_.z;

		return dx * dx + dy * dy + dz * dz;
	}

	bool IsDitherFadeActiveForOutline(const SceneObject& owner, const Camera3d& camera) {
		if(!owner.IsCameraDitherEnabled()) return false;
		const float fadeFarSq = kDitherFadeFar * kDitherFadeFar;
		return DistanceSqPointAabb(camera.GetWorldTransform().GetWorldPosition(), owner.GetWorldAABB()) < fadeFarSq;
	}

	bool IsOutlineEnabledForCameraDither(const SceneObject& owner, const Camera3d& camera) {
		return owner.IsOutlineEnabled() && !IsDitherFadeActiveForOutline(owner, camera);
	}

	bool IsDitherEnabledForTarget(const IRenderTarget& rt) {
		return rt.GetRenderTargetType() != RenderTargetType::DebugView;
	}
}

void OutlineRenderer::Render(ID3D12GraphicsCommandList* cmdList,
							 ID3D12Device*				device,
							 IRenderTarget*				rt,
							 PipelineService*			psoService,
							 const Camera3d*			camera,
							 const ModelRenderer&		modelRenderer) {
	if(!cmdList || !device || !rt || !psoService || !camera) return;

	// 出力Viewportと内部Bufferの解像度を揃え、輪郭Sample位置のずれを防ぐ。
	const auto viewport = rt->GetViewport();
	EnsureResources(device, viewport);
	compositeConstants_ = CompositeConstants{};
	// 対象が存在する場合だけCompositeし、空の場合は呼び出し元のRenderTargetを復元する。
	if(RenderNormalBuffer(cmdList, device, rt, psoService, camera, modelRenderer)) {
		Composite(cmdList, rt, psoService);
	} else {
		rt->SetRenderTarget(cmdList);
	}
}

void OutlineRenderer::RenderSelectionHighlight(ID3D12GraphicsCommandList* cmdList,
											   ID3D12Device*			   device,
											   IRenderTarget*			   rt,
											   PipelineService*		   psoService,
											   const Camera3d*		   camera,
											   const ModelRenderer&	   modelRenderer,
											   const SceneObject*		   selected) {
	if(!cmdList || !device || !rt || !psoService || !camera || !selected) return;

	const auto viewport = rt->GetViewport();
	EnsureResources(device, viewport);
	// Editor選択表示はObject固有設定より優先し、一定の橙色と太さで識別可能にする。
	compositeConstants_ = CompositeConstants{};
	compositeConstants_.color = {1.0f, 0.5f, 0.0f, 1.0f};
	compositeConstants_.thickness = 2.0f;
	compositeConstants_.insideMaskOnly = 1.0f;
	if(RenderNormalBuffer(cmdList, device, rt, psoService, camera, modelRenderer, selected)) {
		Composite(cmdList, rt, psoService);
	} else {
		rt->SetRenderTarget(cmdList);
	}
}

void OutlineRenderer::RenderSelectionHighlight(ID3D12GraphicsCommandList* cmdList,
											   ID3D12Device*			   device,
											   IRenderTarget*			   rt,
											   PipelineService*		   psoService,
											   const Camera3d*		   camera,
											   const ModelRenderer&	   modelRenderer,
											   const std::vector<SceneObject*>& selectedObjects) {
	for(const auto* selected : selectedObjects) {
		RenderSelectionHighlight(cmdList, device, rt, psoService, camera, modelRenderer, selected);
	}
}

void OutlineRenderer::EnsureResources(ID3D12Device* device, const D3D12_VIEWPORT& viewport) {
	const uint32_t width = static_cast<uint32_t>(viewport.Width);
	const uint32_t height = static_cast<uint32_t>(viewport.Height);
	if(width == 0 || height == 0) return;
	// 解像度とResourceが有効なら再確保せず、毎フレームのGPUメモリ再生成を避ける。
	if(width_ == width && height_ == height && normalResource_.Get() && compositeResource_.Get()) return;

	width_ = width;
	height_ = height;

	// Descriptor HandleはResizeを跨いで保持し、Heap上の参照位置を安定させる。
	if(!normalRtv_.IsValid()) {
		normalRtv_ = DescriptorAllocator::Allocate(DescriptorUsage::Rtv);
	}
	if(!compositeRtv_.IsValid()) {
		compositeRtv_ = DescriptorAllocator::Allocate(DescriptorUsage::Rtv);
	}
	if(!selectionDepthDsv_.IsValid()) {
		selectionDepthDsv_ = DescriptorAllocator::Allocate(DescriptorUsage::Dsv);
	}

	// Normal Bufferは符号付き法線を0～1へEncodeするため、無効領域を中央値で初期化する。
	const float clearNormal[] = {0.5f, 0.5f, 0.5f, 0.0f};
	normalResource_.InitializeAsRenderTarget(device, width_, height_, kNormalFormat, L"OutlineNormal", clearNormal);
	normalResource_.CreateRTV(device, normalRtv_.cpu);
	// SRVは初回のみ確保し、Resize時は同一Descriptorを新Resourceへ差し替える。
	if(!normalResource_.GetSRVGpuHandle().ptr) {
		normalResource_.CreateSRV(device);
	} else {
		normalResource_.UpdateSRV(device);
	}
	normalResource_.SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Composite結果は最終ColorへAlpha合成できる標準UNORM形式で保持する。
	compositeResource_.InitializeAsRenderTarget(device, width_, height_, DXGI_FORMAT_R8G8B8A8_UNORM, L"OutlineComposite");
	compositeResource_.CreateRTV(device, compositeRtv_.cpu);
	if(!compositeResource_.GetSRVGpuHandle().ptr) {
		compositeResource_.CreateSRV(device);
	} else {
		compositeResource_.UpdateSRV(device);
	}
	compositeResource_.SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Editor選択輪郭はScene Depthから独立させ、遮蔽状態にかかわらず選択対象全体を抽出する。
	selectionDepthResource_.InitializeAsDepthStencil(device, width_, height_, DXGI_FORMAT_D32_FLOAT, L"OutlineSelectionDepth");
	selectionDepthResource_.CreateDSV(device, selectionDepthDsv_.cpu);
	selectionDepthResource_.SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

bool OutlineRenderer::RenderNormalBuffer(ID3D12GraphicsCommandList* cmdList,
										 ID3D12Device*				 device,
										 IRenderTarget*				 rt,
										 PipelineService*			 psoService,
										 const Camera3d*			 camera,
										 const ModelRenderer&		 modelRenderer,
										 const SceneObject*			 targetOwner) {
	// 前回CompositeでSRV利用したNormal Bufferを書き込み状態へ戻す。
	normalResource_.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	const float clearNormal[] = {0.5f, 0.5f, 0.5f, 0.0f};
	cmdList->ClearRenderTargetView(normalRtv_.cpu, clearNormal, 0, nullptr);

	const auto viewport = rt->GetViewport();
	const auto scissor = MakeRect(width_, height_);
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissor);
	if(targetOwner) {
		// 単一選択表示では専用Depthを消去し、対象自身の前後関係だけを反映する。
		selectionDepthResource_.Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		cmdList->ClearDepthStencilView(selectionDepthDsv_.cpu, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	} else if(IsDitherEnabledForTarget(*rt)) {
		// Camera近傍Dither対象をDepthへ先行描画し、Fade中に輪郭だけが残ることを防ぐ。
		RenderDitherDepthOccluders(cmdList, device, rt, psoService, camera, modelRenderer);
	}
	auto dsv = targetOwner ? selectionDepthDsv_.cpu : rt->GetDSV();
	cmdList->OMSetRenderTargets(1, &normalRtv_.cpu, FALSE, &dsv);

	// Rendererが収集済みの可視Instanceを再利用し、Scene全体の再走査を避ける。
	std::vector<ModelRenderer::RenderInstance> staticInstances;
	std::vector<ModelRenderer::RenderInstance> skinnedInstances;
	modelRenderer.CollectVisibleStatic(staticInstances);
	modelRenderer.CollectVisibleSkinned(skinnedInstances);
	bool hasOutlineSettings = false;
	bool drewAny = false;

	// 複数対象を同時描画する場合は最大Thicknessを採用し、細い輪郭による欠落を防ぐ。
	auto applyOwnerSettings = [&](const SceneObject& owner) {
		const auto& settings = owner.GetOutlineSettings();
		if(!hasOutlineSettings) {
			compositeConstants_.color = settings.color;
			compositeConstants_.thickness = settings.thickness;
			hasOutlineSettings = true;
		} else {
			compositeConstants_.thickness = (std::max)(compositeConstants_.thickness, settings.thickness);
		}
	};

	// 同じModelを共有するStatic InstanceをBatch化し、Outline PassのDraw Callを抑える。
	std::vector<StaticBatch> staticBatches;
	for(const auto& inst : staticInstances) {
		if(!inst.model || !inst.transform || !inst.owner) continue;
		if(targetOwner) {
			if(inst.owner != targetOwner) continue;
		} else if(IsDitherEnabledForTarget(*rt) ? !IsOutlineEnabledForCameraDither(*inst.owner, *camera) : !inst.owner->IsOutlineEnabled()) {
			continue;
		}
		if(!inst.model->GetModelData() || !inst.model->GetIsDrawEnable()) continue;
		if(!targetOwner) applyOwnerSettings(*inst.owner);

		auto it = std::find_if(staticBatches.begin(), staticBatches.end(),
							   [&](const StaticBatch& batch) { return batch.model == inst.model; });
		if(it == staticBatches.end()) {
			StaticBatch batch;
			batch.model = inst.model;
			staticBatches.emplace_back(std::move(batch));
			it = std::prev(staticBatches.end());
		}

		it->transforms.push_back(*inst.transform);
		GpuBillboardParams billboard{};
		billboard.mode = static_cast<uint32_t>(inst.billboardMode);
		it->billboards.push_back(billboard);
	}

	if(!staticBatches.empty()) {
		const auto ps = psoService->GetPipelineSet(PipelineTag::Object::OutlineNormalObject3D, BlendMode::NONE);
		psoService->SetCommand(ps, cmdList);
		camera->SetRootCommand(cmdList, 4);
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		for(auto& batch : staticBatches) {
			const UINT count = static_cast<UINT>(batch.transforms.size());
			if(count == 0) continue;

			batch.model->EnsureInstanceCapacity(device, count);
			batch.model->UploadInstanceMatrices(batch.transforms);
			const auto instanceSrv = batch.model->GetInstanceSrv();
			if(instanceSrv.ptr == 0) continue;
			cmdList->SetGraphicsRootDescriptorTable(1, instanceSrv);

			batch.model->EnsureBillboardCapacity(device, count);
			batch.model->UploadBillboardParams(batch.billboards);
			const auto billboardSrv = batch.model->GetBillboardSrv();
			if(billboardSrv.ptr == 0) continue;
			cmdList->SetGraphicsRootDescriptorTable(7, billboardSrv);

			batch.model->BindVertexIndexBuffers(cmdList);
			cmdList->DrawIndexedInstanced(
				static_cast<UINT>(batch.model->GetModelData()->meshResource.Indices().size()),
				count,
				0,
				0,
				0);
			drewAny = true;
		}
	}

	if(skinnedInstances.empty()) return drewAny;

	bool skinnedPipelineSet = false;
	for(const auto& inst : skinnedInstances) {
		if(!inst.model || !inst.transform || !inst.owner) continue;
		if(targetOwner) {
			if(inst.owner != targetOwner) continue;
		} else if(IsDitherEnabledForTarget(*rt) ? !IsOutlineEnabledForCameraDither(*inst.owner, *camera) : !inst.owner->IsOutlineEnabled()) {
			continue;
		}
		if(!inst.model->GetModelData() || !inst.model->GetIsDrawEnable()) continue;
		if(!targetOwner) applyOwnerSettings(*inst.owner);

		if(!skinnedPipelineSet) {
			const auto ps = psoService->GetPipelineSet(PipelineTag::Object::OutlineNormalSkinnedObject3D, BlendMode::NONE);
			psoService->SetCommand(ps, cmdList);
			camera->SetRootCommand(cmdList, 4);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			skinnedPipelineSet = true;
		}

		auto* model = static_cast<CalyxEngine::AnimationModel*>(inst.model);
		inst.transform->SetCommand(cmdList, 1);
		model->SetCommandPalletSrv(7, cmdList);
		model->BindVertexIndexBuffers(cmdList);
		cmdList->DrawIndexedInstanced(
			static_cast<UINT>(model->GetModelData()->meshResource.Indices().size()),
			1,
			0,
			0,
			0);
		drewAny = true;
	}

	return drewAny;
}

void OutlineRenderer::RenderDitherDepthOccluders(ID3D12GraphicsCommandList* cmdList,
												 ID3D12Device* device,
												 IRenderTarget* rt,
												 PipelineService* psoService,
												 const Camera3d* camera,
												 const ModelRenderer& modelRenderer) {
	if(!cmdList || !device || !rt || !psoService || !camera) return;

	auto dsv = rt->GetDSV();
	cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);

	std::vector<ModelRenderer::RenderInstance> staticInstances;
	std::vector<ModelRenderer::RenderInstance> skinnedInstances;
	modelRenderer.CollectVisibleStatic(staticInstances);
	modelRenderer.CollectVisibleSkinned(skinnedInstances);

	std::vector<StaticBatch> staticBatches;
	for(const auto& inst : staticInstances) {
		if(!inst.model || !inst.transform || !inst.owner) continue;
		if(!IsDitherFadeActiveForOutline(*inst.owner, *camera)) continue;
		if(!inst.model->GetModelData() || !inst.model->GetIsDrawEnable()) continue;

		auto it = std::find_if(staticBatches.begin(), staticBatches.end(),
							   [&](const StaticBatch& batch) { return batch.model == inst.model; });
		if(it == staticBatches.end()) {
			StaticBatch batch;
			batch.model = inst.model;
			staticBatches.emplace_back(std::move(batch));
			it = std::prev(staticBatches.end());
		}

		it->transforms.push_back(*inst.transform);
		GpuBillboardParams billboard{};
		billboard.mode = static_cast<uint32_t>(inst.billboardMode);
		it->billboards.push_back(billboard);
	}

	if(!staticBatches.empty()) {
		const auto ps = psoService->GetPipelineSet(PipelineTag::Object::OutlineDitherDepthObject3D, BlendMode::NONE);
		psoService->SetCommand(ps, cmdList);
		camera->SetRootCommand(cmdList, 4);
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		for(auto& batch : staticBatches) {
			const UINT count = static_cast<UINT>(batch.transforms.size());
			if(count == 0) continue;

			batch.model->EnsureInstanceCapacity(device, count);
			batch.model->UploadInstanceMatrices(batch.transforms);
			const auto instanceSrv = batch.model->GetInstanceSrv();
			if(instanceSrv.ptr == 0) continue;
			cmdList->SetGraphicsRootDescriptorTable(1, instanceSrv);

			batch.model->EnsureBillboardCapacity(device, count);
			batch.model->UploadBillboardParams(batch.billboards);
			const auto billboardSrv = batch.model->GetBillboardSrv();
			if(billboardSrv.ptr == 0) continue;
			cmdList->SetGraphicsRootDescriptorTable(7, billboardSrv);

			batch.model->BindVertexIndexBuffers(cmdList);
			cmdList->DrawIndexedInstanced(
				static_cast<UINT>(batch.model->GetModelData()->meshResource.Indices().size()),
				count,
				0,
				0,
				0);
		}
	}

	bool skinnedPipelineSet = false;
	for(const auto& inst : skinnedInstances) {
		if(!inst.model || !inst.transform || !inst.owner) continue;
		if(!IsDitherFadeActiveForOutline(*inst.owner, *camera)) continue;
		if(!inst.model->GetModelData() || !inst.model->GetIsDrawEnable()) continue;

		if(!skinnedPipelineSet) {
			const auto ps = psoService->GetPipelineSet(PipelineTag::Object::OutlineDitherDepthSkinnedObject3D, BlendMode::NONE);
			psoService->SetCommand(ps, cmdList);
			camera->SetRootCommand(cmdList, 4);
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			skinnedPipelineSet = true;
		}

		auto* model = static_cast<CalyxEngine::AnimationModel*>(inst.model);
		inst.transform->SetCommand(cmdList, 1);
		model->SetCommandPalletSrv(7, cmdList);
		model->BindVertexIndexBuffers(cmdList);
		cmdList->DrawIndexedInstanced(
			static_cast<UINT>(model->GetModelData()->meshResource.Indices().size()),
			1,
			0,
			0,
			0);
	}
}

void OutlineRenderer::Composite(ID3D12GraphicsCommandList* cmdList,
								IRenderTarget*				rt,
								PipelineService*			psoService) {
	normalResource_.Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	rt->TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	rt->TransitionDepthTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	compositeResource_.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	const auto viewport = rt->GetViewport();
	const auto scissor = MakeRect(width_, height_);
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissor);
	cmdList->OMSetRenderTargets(1, &compositeRtv_.cpu, FALSE, nullptr);

	const auto compositePs = psoService->GetPipelineSet(PipelineTag::PostProcess::OutlineComposite);
	psoService->SetCommand(compositePs, cmdList);

	CompositeConstants constants = compositeConstants_;
	constants.texelSize = {1.0f / static_cast<float>(width_), 1.0f / static_cast<float>(height_)};
	constants.thickness = std::clamp(constants.thickness, 1.0f, 4.0f);

	cmdList->SetGraphicsRootDescriptorTable(0, rt->GetSRV());
	cmdList->SetGraphicsRootDescriptorTable(1, rt->GetDepthSRV());
	cmdList->SetGraphicsRootDescriptorTable(2, normalResource_.GetSRVGpuHandle());
	cmdList->SetGraphicsRoot32BitConstants(3, sizeof(CompositeConstants) / sizeof(uint32_t), &constants, 0);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(3, 1, 0, 0);

	compositeResource_.Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	rt->TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	rt->TransitionDepthTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	rt->SetRenderTarget(cmdList);

	const auto copyPs = psoService->GetPipelineSet(PipelineTag::PostProcess::CopyImage);
	psoService->SetCommand(copyPs, cmdList);
	cmdList->SetGraphicsRootDescriptorTable(0, compositeResource_.GetSRVGpuHandle());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(3, 1, 0, 0);
}
