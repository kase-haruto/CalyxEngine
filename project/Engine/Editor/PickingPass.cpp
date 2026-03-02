#include "PickingPass.h"

// engine
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>

// d3d
#include "Engine/Assets/Animation/AnimationModel.h"
#include "Engine/Assets/Model/BaseModel.h"
#include "Engine/Graphics/Camera/3d/Camera3d.h"
#include "Engine/Graphics/Camera/Manager/CameraManager.h"

#include <d3dx12.h>
#include <stdexcept>

using namespace CalyxEditor;

namespace {
	inline void ThrowIfFailed(HRESULT hr) {
		if(FAILED(hr)) {
			throw std::runtime_error("PickingPath : D3D12 call failed.");
		}
	}
} // namespace

/////////////////////////////////////////////////////////////////////////////////////////
// 初期化
/////////////////////////////////////////////////////////////////////////////////////////
void PickingPass::Initialize(int32_t width, int32_t height) {
	// Descriptor 確保
	rtv_ = DescriptorAllocator::Allocate(DescriptorUsage::Rtv);
	dsv_ = DescriptorAllocator::Allocate(DescriptorUsage::Dsv);
	srv_ = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);

	CreateResources(static_cast<uint32_t>(width),
					static_cast<uint32_t>(height));
}

/////////////////////////////////////////////////////////////////////////////////////////
// 終了
/////////////////////////////////////////////////////////////////////////////////////////
void PickingPass::Finalize() {
	DestroyResources();

	DescriptorAllocator::Free(DescriptorUsage::Rtv, rtv_);
	DescriptorAllocator::Free(DescriptorUsage::Dsv, dsv_);
	DescriptorAllocator::Free(DescriptorUsage::CbvSrvUav, srv_);

	rtv_ = {};
	dsv_ = {};
	srv_ = {};
}

/////////////////////////////////////////////////////////////////////////////////////////
// オブジェクトID取得
/////////////////////////////////////////////////////////////////////////////////////////
uint32_t PickingPass::GetObjectID(int32_t x, int32_t y) {
	if(!readback_ || x < 0 || x >= (int)width_ || y < 0 || y >= (int)height_) return 0;

	void*	ptr = nullptr;
	HRESULT hr	= readback_->Map(0, nullptr, &ptr);
	if(FAILED(hr) || !ptr) return 0;

	// 指定座標のピクセルを抽出 (RowPitchを考慮)
	uint8_t* pixelPtr = (uint8_t*)ptr + (y * rowPitch_) + (x * 4);

	uint32_t id = 0;
	id |= static_cast<uint32_t>(pixelPtr[0]) << 0;
	id |= static_cast<uint32_t>(pixelPtr[1]) << 8;
	id |= static_cast<uint32_t>(pixelPtr[2]) << 16;
	// Alpha(pixelPtr[3]) は ID に含めない (0xFF が入っているため比較が壊れる)

	readback_->Unmap(0, nullptr);
	return id;
}

/////////////////////////////////////////////////////////////////////////////////////////
// デプス取得
/////////////////////////////////////////////////////////////////////////////////////////
float PickingPass::GetDepth(int32_t x, int32_t y) {
	if(!readbackDepth_ || x < 0 || x >= (int)width_ || y < 0 || y >= (int)height_) return 1.0f;

	void*	ptr = nullptr;
	HRESULT hr	= readbackDepth_->Map(0, nullptr, &ptr);
	if(FAILED(hr) || !ptr) return 1.0f;

	// 指定座標のピクセルを抽出 (RowPitchを考慮)
	float* depthPtr = (float*)((uint8_t*)ptr + (y * rowPitchDepth_) + (x * 4));
	float  depth	= *depthPtr;

	readbackDepth_->Unmap(0, nullptr);
	return depth;
}

/////////////////////////////////////////////////////////////////////////////////////////
// リサイズ
/////////////////////////////////////////////////////////////////////////////////////////
void PickingPass::Resize(int32_t width, int32_t height) {
	if(width <= 0 || height <= 0) return;
	if(width_ == static_cast<uint32_t>(width) &&
	   height_ == static_cast<uint32_t>(height)) {
		return;
	}

	DestroyResources();
	CreateResources(static_cast<uint32_t>(width),
					static_cast<uint32_t>(height));
}

/////////////////////////////////////////////////////////////////////////////////////////
// リソース生成
/////////////////////////////////////////////////////////////////////////////////////////
void PickingPass::CreateResources(uint32_t w, uint32_t h) {
	auto device = GraphicsGroup::GetInstance()->GetDevice().Get();
	if(!device) {
		throw std::runtime_error("PickingPath : device is null.");
	}

	width_	= w;
	height_ = h;

	// =========================================================
	// Color RT（ObjectID）
	// =========================================================
	{
		const DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		D3D12_RESOURCE_DESC desc =
			CD3DX12_RESOURCE_DESC::Tex2D(
				colorFormat,
				width_,
				height_,
				1, 1, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		D3D12_CLEAR_VALUE clear{};
		clear.Format   = colorFormat;
		clear.Color[0] = 0.0f;
		clear.Color[1] = 0.0f;
		clear.Color[2] = 0.0f;
		clear.Color[3] = 0.0f;

		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(
			device->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&clear,
				IID_PPV_ARGS(&color_)));

		device->CreateRenderTargetView(color_.Get(), nullptr, rtv_.cpu);
		device->CreateShaderResourceView(color_.Get(), nullptr, srv_.cpu);
	}

	// =========================================================
	// Depth
	// =========================================================
	{
		const DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;

		D3D12_RESOURCE_DESC desc =
			CD3DX12_RESOURCE_DESC::Tex2D(
				depthFormat,
				width_,
				height_,
				1, 1, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		D3D12_CLEAR_VALUE clear{};
		clear.Format			   = depthFormat;
		clear.DepthStencil.Depth   = 1.0f;
		clear.DepthStencil.Stencil = 0;

		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(
			device->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&clear,
				IID_PPV_ARGS(&depth_)));

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format		  = depthFormat;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

		device->CreateDepthStencilView(depth_.Get(), &dsvDesc, dsv_.cpu);
	}

	// =========================================================
	// Readback Buffer
	// =========================================================
	CreateReadback(width_, height_);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Readback生成
/////////////////////////////////////////////////////////////////////////////////////////
void PickingPass::CreateReadback(uint32_t w, uint32_t h) {
	auto device = GraphicsGroup::GetInstance()->GetDevice().Get();

	UINT64							   totalSize = 0;
	D3D12_RESOURCE_DESC				   desc		 = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, w, h);
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
	device->GetCopyableFootprints(&desc, 0, 1, 0, &layout, nullptr, nullptr, &totalSize);

	rowPitch_ = layout.Footprint.RowPitch;

	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
	auto resDesc   = CD3DX12_RESOURCE_DESC::Buffer(totalSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&readback_)));

	// Depth Readback
	{
		D3D12_RESOURCE_DESC descDepth = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, w, h);
		device->GetCopyableFootprints(&descDepth, 0, 1, 0, &layout, nullptr, nullptr, &totalSize);
		rowPitchDepth_ = layout.Footprint.RowPitch;

		auto resDescDepth = CD3DX12_RESOURCE_DESC::Buffer(totalSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDescDepth,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&readbackDepth_)));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// リソース破棄
/////////////////////////////////////////////////////////////////////////////////////////
void PickingPass::DestroyResources() {
	color_.Reset();
	depth_.Reset();
	readback_.Reset();
	readbackDepth_.Reset();
	width_ = height_ = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// 描画
/////////////////////////////////////////////////////////////////////////////////////////
void PickingPass::Render(
	ID3D12GraphicsCommandList* cmd,
	ModelRenderer*			   modelRenderer,
	PipelineService*		   psoService) {
	if(!cmd || !modelRenderer || !psoService) return;

	auto device = GraphicsGroup::GetInstance()->GetDevice().Get();

	// エディタでのピッキングは基本的にデバッグカメラの視点を基準にする
	auto camera = CameraManager::GetDebug();
	if(!camera) return;

	// =========================================================
	// State Transition
	// =========================================================
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			color_.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmd->ResourceBarrier(1, &barrier);
	}

	// =========================================================
	// OM 設定
	// =========================================================
	cmd->OMSetRenderTargets(1, &rtv_.cpu, FALSE, &dsv_.cpu);

	// =========================================================
	// Clear
	// =========================================================
	const float clearColor[4] = {0, 0, 0, 0};
	cmd->ClearRenderTargetView(rtv_.cpu, clearColor, 0, nullptr);
	cmd->ClearDepthStencilView(
		dsv_.cpu,
		D3D12_CLEAR_FLAG_DEPTH,
		1.0f, 0, 0, nullptr);

	// =========================================================
	// Viewport / Scissor
	// =========================================================
	D3D12_VIEWPORT vp{};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width	= static_cast<float>(width_);
	vp.Height	= static_cast<float>(height_);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	D3D12_RECT sc{};
	sc.left	  = 0;
	sc.top	  = 0;
	sc.right  = static_cast<LONG>(width_);
	sc.bottom = static_cast<LONG>(height_);

	cmd->RSSetViewports(1, &vp);
	cmd->RSSetScissorRects(1, &sc);

	// =========================================================
	// Static Mesh Picking
	// =========================================================
	{
		std::vector<ModelRenderer::RenderInstance> instances;
		modelRenderer->CollectVisibleStatic(instances);

		const auto ps = psoService->GetPipelineSet(
			PipelineTag::Object::PickingObject3D,
			BlendMode::NONE);
		psoService->SetCommand(ps, cmd);

		// [2] Camera
		camera->SetRootCommand(cmd, 2);

		for(auto& inst : instances) {
			if(!inst.owner->IsPickable() || inst.owner->IsTransient()) continue;

			// [0] Picking ID
			cmd->SetGraphicsRoot32BitConstant(0, inst.owner->GetPickingID(), 0);

			inst.model->BindVertexIndexBuffers(cmd);
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// [1] World Transform
			inst.transform->SetCommand(cmd, 1);

			// [3] Billboard (Default None)
			inst.model->EnsureBillboardCapacity(device, 1);
			std::vector<GpuBillboardParams> bb{};
			inst.model->UploadBillboardParams(bb);
			cmd->SetGraphicsRootDescriptorTable(3, inst.model->GetBillboardSrv());

			cmd->DrawIndexedInstanced(
				static_cast<UINT>(inst.model->GetModelData()->meshResource.Indices().size()),
				1, 0, 0, 0);
		}
	}

	// =========================================================
	// Skinned Mesh Picking
	// =========================================================
	{
		std::vector<ModelRenderer::RenderInstance> instances;
		modelRenderer->CollectVisibleSkinned(instances);

		const auto ps = psoService->GetPipelineSet(
			PipelineTag::Object::PickingSkinned,
			BlendMode::NONE);
		psoService->SetCommand(ps, cmd);

		// [2] Camera
		camera->SetRootCommand(cmd, 2);

		for(auto& inst : instances) {
			if(!inst.owner->IsPickable() || inst.owner->IsTransient()) continue;

			// [0] Picking ID
			cmd->SetGraphicsRoot32BitConstant(0, inst.owner->GetPickingID(), 0);

			inst.model->BindVertexIndexBuffers(cmd);
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// [1] World Transform (CBV)
			inst.transform->SetCommand(cmd, 1);

			// [3] Skinning Buffer (SRV Table)
			if(auto* animModel = dynamic_cast<CalyxAssets::AnimationModel*>(inst.model)) {
				animModel->SetCommandPalletSrv(3, cmd);
			}

			cmd->DrawIndexedInstanced(
				static_cast<UINT>(inst.model->GetModelData()->meshResource.Indices().size()),
				1, 0, 0, 0);
		}
	}

	// =========================================================
	// Readback Copy
	// =========================================================
	if(readback_) {
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			color_.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
		cmd->ResourceBarrier(1, &barrier);

		D3D12_RESOURCE_DESC				   desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width_, height_);
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
		device->GetCopyableFootprints(&desc, 0, 1, 0, &layout, nullptr, nullptr, nullptr);

		CD3DX12_TEXTURE_COPY_LOCATION dst(readback_.Get(), layout);
		CD3DX12_TEXTURE_COPY_LOCATION src(color_.Get(), 0);
		cmd->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			color_.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmd->ResourceBarrier(1, &barrier);
	}

	// =========================================================
	// Depth Readback Copy
	// =========================================================
	if(readbackDepth_) {
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			depth_.Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
		cmd->ResourceBarrier(1, &barrier);

		D3D12_RESOURCE_DESC				   desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width_, height_);
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
		device->GetCopyableFootprints(&desc, 0, 1, 0, &layout, nullptr, nullptr, nullptr);

		CD3DX12_TEXTURE_COPY_LOCATION dst(readbackDepth_.Get(), layout);
		CD3DX12_TEXTURE_COPY_LOCATION src(depth_.Get(), 0);
		cmd->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			depth_.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
		cmd->ResourceBarrier(1, &barrier);
	}
}
