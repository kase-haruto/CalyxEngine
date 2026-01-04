#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/Platform/WinApp.h>
#include <Engine/Graphics/Device/DxDevice.h>
#include <Engine/Graphics/Context/DxCommand.h>
#include <Engine/Graphics/Context/DxFence.h>
#include <Engine/Graphics/SwapChain/DxSwapChain.h>
#include <Engine/Graphics/RenderTarget/Collection/RenderTargetCollection.h>
#include <Engine/Graphics/ResourceStateTracker/ResourceStateTracker.h>

// c++
#include <memory>

using Microsoft::WRL::ComPtr;

class DxCore{
public:
	//===================================================================*/
	//		public methods
	//===================================================================*/
	DxCore() = default;
	~DxCore();

	// 初期化
	void Initialize(WinApp* winApp, uint32_t width, uint32_t height);

	// オフスクリーンレンダラー初期化
	void RendererInitialize(uint32_t width, uint32_t height);

	// 描画処理
	void PreDraw();
	void PreDrawOffscreen();
	void DrawOffscreenTexture();
	void PostDraw();
	void RenderEngineUI();

private:
	//===================================================================*/
	//		private methods
	//===================================================================*/

	// リソース解放
	void ReleaseResources();

public:
	//===================================================================*/
	//		accessor
	//===================================================================*/
	const ComPtr<ID3D12Device>& GetDevice() const{ return dxDevice_->GetDevice(); }
	const ComPtr<ID3D12GraphicsCommandList>& GetCommandList() const{ return dxCommand_->GetCommandList(); }
	const DxSwapChain& GetSwapChain() const{ return *dxSwapChain_; }
	const RenderTargetCollection& GetRenderTargetCollection() const{ return *renderTargetCollection_; }
	// メソッド追加
	DXGI_FORMAT GetFormat() const{ return format_; }
private:
	//===================================================================*/
	//		private methods
	//===================================================================*/

	WinApp* winApp_ = nullptr;
	uint32_t clientWidth_ = 0;
	uint32_t clientHeight_ = 0;


	// DirectX関連
	std::unique_ptr<DxDevice> dxDevice_;
	std::unique_ptr<DxCommand> dxCommand_;
	std::unique_ptr<DxSwapChain> dxSwapChain_;
	
	ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	ComPtr<ID3D12DescriptorHeap> dsvHeap_;
	UINT rtvDescriptorSize_ = 0;
	DXGI_FORMAT format_ = DXGI_FORMAT_R8G8B8A8_UNORM; // 仮の初期値
	std::unique_ptr<RenderTargetCollection> renderTargetCollection_;
	std::unique_ptr<DxFence> dxFence_;

};
