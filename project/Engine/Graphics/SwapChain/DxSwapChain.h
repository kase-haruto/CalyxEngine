#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>
#include <cassert>
#include<chrono>


class DxSwapChain{
    template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    void Initialize(ComPtr<IDXGIFactory7> dxgiFactory, ComPtr<ID3D12CommandQueue> commandQueue, HWND hwnd, uint32_t width, uint32_t height);

    void Present();


    /////////////////////////////////////////////////////////////////////////////////////////
    //              アクセッサ
    ///////////////////////////////////////////////////////////////////////////////////////// 
public:
    UINT GetCurrentBackBufferIndex() const{ return swapChain_->GetCurrentBackBufferIndex(); }
    ComPtr<IDXGISwapChain4> GetSwapChain() const{ return swapChain_; }
    ComPtr<ID3D12Resource> GetBackBuffer(UINT index) const{ return backBuffers_[index]; }
    void SetBackBuffer(uint32_t index, ComPtr<ID3D12Resource>bb){ backBuffers_[index] = bb; }
    DXGI_SWAP_CHAIN_DESC1 GetSwapChainDesc()const{ return swapChainDesc_; }

private:
    ///////////////////////////////////////////////////
    //              リソース
    ///////////////////////////////////////////////////
    ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
    std::array<ComPtr<ID3D12Resource>, 2> backBuffers_;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc_ {};

    float refreshRate_ = 60.0f; // デフォルトで60Hz
	UINT syncInterval_ = 1; // デフォルトで垂直同期を行う

};
