#include "DxCore.h"

#include <Engine/Application/System/Environment.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/RenderTarget/OffscreenRT/OffscreenRenderTarget.h>
#include <Engine/Graphics/RenderTarget/SwapChainRT/SwapChainRenderTarget.h>
#include <Engine/PostProcess/FullscreenDrawer.h>

#include <cassert>
#include <d3dx12.h>
#include <dxgidebug.h>
#include <externals/imgui/imgui.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

namespace CalyxGraphics {
	
	DxCore::~DxCore() {
		ReleaseResources();
	}

	void DxCore::ReleaseResources() {
		dxSwapChain_.reset();
		dxFence_.reset();
		dxCommand_.reset();
		dxDevice_.reset();
	}

	void DxCore::Initialize(WinApp* winApp, uint32_t width, uint32_t height) {
		winApp_		  = winApp;
		clientWidth_  = width;
		clientHeight_ = height;

		// 各種DirectX関連インスタンスの生成
		dxDevice_	 = std::make_unique<DxDevice>();
		dxCommand_	 = std::make_unique<DxCommand>();
		dxSwapChain_ = std::make_unique<DxSwapChain>();
		dxFence_	 = std::make_unique<DxFence>();

		// 初期化処理
		dxDevice_->Initialize();
		dxCommand_->Initialize(dxDevice_->GetDevice());
		dxSwapChain_->Initialize(dxDevice_->GetDXGIFactory(), dxCommand_->GetCommandQueue(), winApp_->GetHWND(), width, height);
		dxFence_->Initialize(dxDevice_->GetDevice());
	}

	void DxCore::RendererInitialize(uint32_t width, uint32_t height) {
		renderTargetCollection_ = std::make_unique<RenderTargetCollection>();
		auto device				= dxDevice_->GetDevice();

		// SwapChainRT
		auto swapchainRT = std::make_unique<SwapChainRenderTarget>();
		swapchainRT->Initialize(dxSwapChain_.get());
		swapchainRT->SetRenderTargetType(RenderTargetType::BackBuffer);
		renderTargetCollection_->Add("BackBuffer", std::move(swapchainRT));

		// Offscreen
		auto offscreenRT = std::make_unique<OffscreenRenderTarget>();
		offscreenRT->Initialize(device.Get(), width, height, format_, DescriptorAllocator::Allocate(DescriptorUsage::Rtv), DescriptorAllocator::Allocate(DescriptorUsage::Dsv));
		offscreenRT->SetRenderTargetType(RenderTargetType::Offscreen);
		renderTargetCollection_->Add("Offscreen", std::move(offscreenRT));

		// PostEffect
		auto postEffectRT = std::make_unique<OffscreenRenderTarget>();
		postEffectRT->Initialize(device.Get(), width, height, format_, DescriptorAllocator::Allocate(DescriptorUsage::Rtv), DescriptorAllocator::Allocate(DescriptorUsage::Dsv));
		postEffectRT->SetRenderTargetType(RenderTargetType::PostEffectOutput);
		renderTargetCollection_->Add("PostEffectOutput", std::move(postEffectRT));

		// Debug
		auto debugRT = std::make_unique<OffscreenRenderTarget>();
		debugRT->Initialize(device.Get(), width, height, format_, DescriptorAllocator::Allocate(DescriptorUsage::Rtv), DescriptorAllocator::Allocate(DescriptorUsage::Dsv));
		debugRT->SetRenderTargetType(RenderTargetType::DebugView);
		renderTargetCollection_->Add("DebugView", std::move(debugRT));

		// Ping-Pong Buffers
		auto postBuffer1 = std::make_unique<OffscreenRenderTarget>();
		postBuffer1->Initialize(device.Get(), width, height, format_, DescriptorAllocator::Allocate(DescriptorUsage::Rtv), DescriptorAllocator::Allocate(DescriptorUsage::Dsv));
		postBuffer1->SetRenderTargetType(RenderTargetType::PostEffectBuffer1);
		renderTargetCollection_->Add("PostEffectBuffer1", std::move(postBuffer1));

		auto postBuffer2 = std::make_unique<OffscreenRenderTarget>();
		postBuffer2->Initialize(device.Get(), width, height, format_, DescriptorAllocator::Allocate(DescriptorUsage::Rtv), DescriptorAllocator::Allocate(DescriptorUsage::Dsv));
		postBuffer2->SetRenderTargetType(RenderTargetType::PostEffectBuffer2);
		renderTargetCollection_->Add("PostEffectBuffer2", std::move(postBuffer2));
	}

	void DxCore::PreDraw() {
		UINT bufferIndex = dxSwapChain_->GetCurrentBackBufferIndex();
		auto commandList = dxCommand_->GetCommandList();

		// SwapChainRenderTarget の設定
		if(auto* swapchainRT = dynamic_cast<SwapChainRenderTarget*>(renderTargetCollection_->Get("BackBuffer"))) {
			swapchainRT->SetBufferIndex(bufferIndex);
			swapchainRT->TransitionTo(commandList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
			swapchainRT->Clear(commandList.Get());
			swapchainRT->SetRenderTarget(commandList.Get());
		}
	}

	void DxCore::PreDrawOffscreen() {
		auto* offscreenTarget = renderTargetCollection_->Get("Offscreen");
		if(offscreenTarget) {
			offscreenTarget->Clear(dxCommand_->GetCommandList().Get());
			offscreenTarget->TransitionTo(dxCommand_->GetCommandList().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
			offscreenTarget->SetRenderTarget(dxCommand_->GetCommandList().Get());
		}
	}

	void DxCore::PostDraw() {
		UINT bufferIndex = dxSwapChain_->GetCurrentBackBufferIndex();
		auto commandList = dxCommand_->GetCommandList();

		if(auto* swapchainRT = dynamic_cast<SwapChainRenderTarget*>(renderTargetCollection_->Get("BackBuffer"))) {
			swapchainRT->SetBufferIndex(bufferIndex);
			swapchainRT->TransitionTo(commandList.Get(), D3D12_RESOURCE_STATE_PRESENT);
		}

		HRESULT hr = commandList->Close();
		if(FAILED(hr)) {
			// ログを出力するか、デバッグ用にブレークポイントを設定
			OutputDebugStringA("commandList->Close() failed.\n");

			// エラーハンドリング
			throw std::runtime_error("Failed to close command list.");
		}

		ID3D12CommandList* commandLists[] = {commandList.Get()};
		dxCommand_->GetCommandQueue()->ExecuteCommandLists(_countof(commandLists), commandLists);

		dxSwapChain_->Present();
		dxFence_->Signal(dxCommand_->GetCommandQueue());
		dxFence_->Wait();
		dxCommand_->Reset();
	}

	void DxCore::Resize(uint32_t width, uint32_t height) {
		if (width == 0 || height == 0) return;
		if (width == clientWidth_ && height == clientHeight_) return;

		clientWidth_ = width;
		clientHeight_ = height;

		// GPUの完了を待つ
		dxFence_->Signal(dxCommand_->GetCommandQueue());
		dxFence_->Wait();

		// コマンドリストの状態をクリアし、参照を外す
		auto commandList = dxCommand_->GetCommandList();
		commandList->ClearState(nullptr);
		commandList->Close();
		dxCommand_->Reset();

		// スワップチェーンをリサイズ (内部で古いバックバッファを解放する)
		dxSwapChain_->Resize(width, height);

		// 全てのレンダーターゲットをリサイズ
		for (auto& pair : renderTargetCollection_->GetMap()) {
			pair.second->Resize(width, height);
		}
	}

	void DxCore::RenderEngineUI() {
#ifdef _DEBUG

		// ImGui::End();
#endif // _DEBUG
	}
	
} // namespace CalyxGraphics