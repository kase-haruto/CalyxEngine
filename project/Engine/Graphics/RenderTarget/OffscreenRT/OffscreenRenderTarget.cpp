#include "OffscreenRenderTarget.h"

#include <Engine/Application/System/Environment.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>

#include <Engine/Foundation/Debug/CxAssert.h>
#include <d3dx12.h>
#include <stdexcept>

void OffscreenRenderTarget::Initialize(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, DescriptorHandle rtvHandle, DescriptorHandle dsvHandle) {
	// Descriptorは外部Allocatorが所有するため、再生成時にも同じスロットへViewを書き戻す。
	rtvHandle_ = rtvHandle;
	dsvHandle_ = dsvHandle;

	// RenderTargetとRasterizerの描画範囲を同じ解像度へ同期する。
	viewport_ = {0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
	scissorRect_ = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};

	// 初回だけWrapperとSRV Descriptorを確保し、Resize時はDescriptor参照を維持して中身を更新する。
	if(!resource_) {
		resource_ = std::make_unique<DxGpuResource>();
		resource_->InitializeAsRenderTarget(device, width, height, format);
		resource_->CreateRTV(device, rtvHandle_.cpu);
		resource_->CreateSRV(device);
	} else {
		resource_->InitializeAsRenderTarget(device, width, height, format);
		resource_->CreateRTV(device, rtvHandle_.cpu);
		resource_->UpdateSRV(device);
	}

	// 作成APIの初期状態とTrackerの認識を一致させ、最初のBarrierを正しく生成する。
	resource_->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);

	// DepthもColorと同じライフタイムで再生成し、Viewportとの解像度不一致を防ぐ。
	if(!depthResource_) {
		depthResource_ = std::make_unique<DxGpuResource>();
		depthResource_->InitializeAsDepthStencil(device, width, height, DXGI_FORMAT_R32_TYPELESS);
		depthResource_->CreateDSV(device, dsvHandle_.cpu);
		depthResource_->CreateSRV(device);
	} else {
		depthResource_->InitializeAsDepthStencil(device, width, height, DXGI_FORMAT_R32_TYPELESS);
		depthResource_->CreateDSV(device, dsvHandle_.cpu);
		depthResource_->UpdateSRV(device);
	}
}

DxGpuResource* OffscreenRenderTarget::GetResource() const {
	if(!mrtResources_.empty()) {
		return mrtResources_[0].get();
	}
	return resource_.get();
}

D3D12_CPU_DESCRIPTOR_HANDLE OffscreenRenderTarget::GetRTV() const {
	return rtvHandle_.cpu;
}

D3D12_CPU_DESCRIPTOR_HANDLE OffscreenRenderTarget::GetDSV() const {
	return dsvHandle_.cpu;
}

D3D12_GPU_DESCRIPTOR_HANDLE OffscreenRenderTarget::GetSRV() const {
	if(!mrtResources_.empty()) {
		return mrtResources_[0]->GetSRVGpuHandle();
	}
	return resource_->GetSRVGpuHandle();
}

D3D12_GPU_DESCRIPTOR_HANDLE OffscreenRenderTarget::GetDepthSRV() const {
	return depthResource_->GetSRVGpuHandle();
}

void OffscreenRenderTarget::SetDepthDSV(DescriptorHandle dsv) {
	dsvHandle_ = dsv;
}

D3D12_VIEWPORT OffscreenRenderTarget::GetViewport() const {
	return viewport_;
}

D3D12_RECT OffscreenRenderTarget::GetScissorRect() const {
	return scissorRect_;
}

void OffscreenRenderTarget::Clear(ID3D12GraphicsCommandList* commandList) {
	const float sceneClearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};
	const float maskClearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};

	if(!mrtResources_.empty()) {
		// Scene Colorは背景色、補助Targetはマスク用途の透明値で個別に初期化する。
		mrtResources_[0]->Transition(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ClearRenderTargetView(mrtRtvHandles_[0].cpu, sceneClearColor, 0, nullptr);

		for(size_t i = 1; i < mrtResources_.size(); ++i) {
			mrtResources_[i]->Transition(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			commandList->ClearRenderTargetView(mrtRtvHandles_[i].cpu, maskClearColor, 0, nullptr);
		}
	} else {
		// 単一Targetの場合もClear前に書き込み可能状態へ遷移させる。
		resource_->Transition(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ClearRenderTargetView(rtvHandle_.cpu, sceneClearColor, 0, nullptr);
	}

	// 前フレームの深度を持ち越さないよう、Depth Write状態へ戻して最大深度で消去する。
	depthResource_->Transition(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	commandList->ClearDepthStencilView(dsvHandle_.cpu, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void OffscreenRenderTarget::SetRenderTarget(ID3D12GraphicsCommandList* commandList) {
	// CommandListへ出力領域を記録してから、単一/MRTに応じたDescriptorを設定する。
	commandList->RSSetViewports(1, &viewport_);
	commandList->RSSetScissorRects(1, &scissorRect_);

	if(!mrtResources_.empty()) {
		SetRenderTargetMRT(commandList);
		return;
	}

	commandList->OMSetRenderTargets(1, &rtvHandle_.cpu, FALSE, &dsvHandle_.cpu);
}

void OffscreenRenderTarget::SetRenderTargetMRT(ID3D12GraphicsCommandList* commandList) {
	commandList->RSSetViewports(1, &viewport_);
	commandList->RSSetScissorRects(1, &scissorRect_);

	// OMSetRenderTargetsは連続したCPU Handle配列を要求するため、管理Handleから一時配列を構築する。
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles;
	rtvHandles.reserve(mrtRtvHandles_.size());
	for(const auto& handle : mrtRtvHandles_) {
		rtvHandles.push_back(handle.cpu);
	}
	commandList->OMSetRenderTargets((UINT)rtvHandles.size(), rtvHandles.data(), FALSE, &dsvHandle_.cpu);
}

void OffscreenRenderTarget::TransitionMRTTo(ID3D12GraphicsCommandList* cmdList, size_t index, D3D12_RESOURCE_STATES newState) {
	if(index >= mrtResources_.size()) return;
	mrtResources_[index]->Transition(cmdList, newState);
}

void OffscreenRenderTarget::TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState) {
	if(!mrtResources_.empty()) {
		mrtResources_[0]->Transition(cmdList, newState);
		return;
	}
	resource_->Transition(cmdList, newState);
}

void OffscreenRenderTarget::TransitionDepthTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState) {
	depthResource_->Transition(cmdList, newState);
}

void OffscreenRenderTarget::Resize(uint32_t width, uint32_t height) {
	auto device = GraphicsGroup::GetInstance()->GetDevice();
	if(!mrtResources_.empty()) {
		// 同一解像度ならGPUリソースを作り直さず、不要なDescriptor更新とGPUメモリ確保を避ける。
		auto desc = mrtResources_[0]->Get()->GetDesc();
		if(desc.Width == width && desc.Height == height) return;

		// MRTごとのFormatを保存して再初期化し、Attachment構成をResize前後で維持する。
		std::vector<DXGI_FORMAT> formats;
		formats.reserve(mrtResources_.size());
		for(const auto& resource : mrtResources_) {
			formats.push_back(resource->Get()->GetDesc().Format);
		}
		InitializeMRT(device.Get(), width, height, formats, mrtRtvHandles_, dsvHandle_);
		return;
	}

	if(!resource_) return;
	DXGI_FORMAT format = resource_->Get()->GetDesc().Format;
	auto desc = resource_->Get()->GetDesc();
	if(desc.Width == width && desc.Height == height) return;

	Initialize(device.Get(), width, height, format, rtvHandle_, dsvHandle_);
}

void OffscreenRenderTarget::InitializeMRT(ID3D12Device* device, uint32_t width, uint32_t height,
										  const std::vector<DXGI_FORMAT>& formats,
										  const std::vector<DescriptorHandle>& rtvHandles,
										  DescriptorHandle dsvHandle) {
	if(formats.empty() || rtvHandles.empty() || formats.size() != rtvHandles.size()) {
		throw std::invalid_argument("MRT formats and rtvHandles must have same non-zero size");
	}

	// Descriptorの所有権は移さず、各Attachmentと対応するHandleの組だけを保持する。
	mrtRtvHandles_ = rtvHandles;
	dsvHandle_ = dsvHandle;

	viewport_ = {0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
	scissorRect_ = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};

	rtvHandle_ = rtvHandles[0];

	// Formatごとに独立したColor ResourceとRTV/SRVを生成する。
	mrtResources_.clear();
	for(size_t i = 0; i < formats.size(); ++i) {
		auto resource = std::make_unique<DxGpuResource>();
		resource->InitializeAsRenderTarget(device, width, height, formats[i]);
		resource->CreateRTV(device, rtvHandles[i].cpu);
		resource->CreateSRV(device);
		resource->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
		mrtResources_.push_back(std::move(resource));
	}

	if(!mrtResources_.empty()) {
		// 既存の単一Target APIから先頭Attachmentを参照できる互換経路を維持する。
		resource_ = std::make_unique<DxGpuResource>(*mrtResources_[0]);
	}

	// 全Color Attachmentで共有するDepth Bufferを同じ解像度へ揃える。
	if(!depthResource_) {
		depthResource_ = std::make_unique<DxGpuResource>();
		depthResource_->InitializeAsDepthStencil(device, width, height, DXGI_FORMAT_R32_TYPELESS);
		depthResource_->CreateDSV(device, dsvHandle_.cpu);
		depthResource_->CreateSRV(device);
	} else {
		depthResource_->InitializeAsDepthStencil(device, width, height, DXGI_FORMAT_R32_TYPELESS);
		depthResource_->CreateDSV(device, dsvHandle_.cpu);
		depthResource_->UpdateSRV(device);
	}
}
