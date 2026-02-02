#include"DxSwapChain.h"

/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/System/Environment.h>

// c++
#include <thread>
#include <algorithm>

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
	assert(SUCCEEDED(hr));

	hr = tempSwapChain.As(&swapChain_);
	assert(SUCCEEDED(hr));

	// モニターのリフレッシュレートを取得
	ComPtr<IDXGIOutput> output;
	hr = swapChain_->GetContainingOutput(&output);

	if (FAILED(hr)){
		OutputDebugStringA("Failed to get containing output. Using default refresh rate.\n");
		refreshRate_ = 60.0f; // デフォルト値
	} else{
		// リフレッシュレート取得
		DXGI_MODE_DESC modeDesc = {};
		modeDesc.Width = width;
		modeDesc.Height = height;
		modeDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		DXGI_MODE_DESC closestMatch = {};
		hr = output->FindClosestMatchingMode(&modeDesc, &closestMatch, nullptr);

		if (FAILED(hr) || closestMatch.RefreshRate.Numerator == 0 || closestMatch.RefreshRate.Denominator == 0){
			OutputDebugStringA("Failed to get refresh rate, using default 60Hz.\n");
			refreshRate_ = 60.0f;
		} else{
			refreshRate_ = static_cast< float >(closestMatch.RefreshRate.Numerator) / closestMatch.RefreshRate.Denominator;
		}
	}

	syncInterval_ = static_cast< UINT >(std::round(refreshRate_ / 60.0f));
	if (syncInterval_ < 1) syncInterval_ = 1;

	// バックバッファリソースを取得
	for (UINT i = 0; i < swapChainDesc_.BufferCount; ++i) {
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i]));
		assert(SUCCEEDED(hr));
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
	assert(SUCCEEDED(hr));

	swapChainDesc_.Width = width;
	swapChainDesc_.Height = height;

	// リソースを再取得
	for (UINT i = 0; i < swapChainDesc_.BufferCount; ++i) {
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i]));
		assert(SUCCEEDED(hr));
	}
}
