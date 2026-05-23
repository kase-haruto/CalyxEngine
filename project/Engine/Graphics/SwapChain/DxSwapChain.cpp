#include"DxSwapChain.h"
#include <Engine/Foundation/Debug/CxAssert.h>

/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/System/Environment.h>

void DxSwapChain::Initialize(
	ComPtr<IDXGIFactory7> dxgiFactory,
	ComPtr<ID3D12CommandQueue> commandQueue,
	HWND hwnd,
	uint32_t width,
	uint32_t height
){
	HRESULT hr;

	// スワップチェイン設定
	swapChainDesc_.Width = width;
	swapChainDesc_.Height = height;
	swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc_.SampleDesc.Count = 1;
	swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc_.BufferCount = 2;
	swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc_.Scaling = DXGI_SCALING_NONE;

	// スワップチェイン作成
	ComPtr<IDXGISwapChain1> tempSwapChain;
	hr = dxgiFactory->CreateSwapChainForHwnd(
		commandQueue.Get(),
		hwnd,
		&swapChainDesc_,
		nullptr,
		nullptr,
		&tempSwapChain
	);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	hr = tempSwapChain.As(&swapChain_);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	// 1 = VSyncあり。現在のモニターリフレッシュレートに同期してティアリングを防ぐ。
	syncInterval_ = 1;

	// バックバッファリソースを取得
	for (UINT i = 0; i < swapChainDesc_.BufferCount; ++i) {
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i]));
		CX_CHECK(SUCCEEDED(hr), "Assertion failed");
	}
}

void DxSwapChain::Present(){
	// スワップチェインをPresent
	swapChain_->Present(syncInterval_, 0);
}

void DxSwapChain::Resize(uint32_t width, uint32_t height) {
	// リソースを確実に解放
	for (auto& bb : backBuffers_) {
		bb = nullptr;
		bb.Reset();
	}

	HRESULT hr = swapChain_->ResizeBuffers(
		swapChainDesc_.BufferCount,
		width,
		height,
		swapChainDesc_.Format,
		swapChainDesc_.Flags
	);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	swapChainDesc_.Width = width;
	swapChainDesc_.Height = height;

	// リソースを再取得
	for (UINT i = 0; i < swapChainDesc_.BufferCount; ++i) {
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i]));
		CX_CHECK(SUCCEEDED(hr), "Assertion failed");
	}
}
