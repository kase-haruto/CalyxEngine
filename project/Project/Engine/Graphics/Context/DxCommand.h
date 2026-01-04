#pragma once

// c++
#include <d3d12.h>
#include <wrl.h>
#include <cassert>


class DxCommand{
    template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="device"></param>
    void Initialize(const ComPtr<ID3D12Device>& device);
    /// <summary>
    /// リセット
    /// </summary>
    void Reset();

    /////////////////////////////////////////////////////////////////////////////////////////
    //              アクセッサ
    /////////////////////////////////////////////////////////////////////////////////////////    ComPtr<ID3D12GraphicsCommandList> GetCommandList() const{ return commandList_; }
    const ComPtr<ID3D12GraphicsCommandList>& GetCommandList() const{ return commandList_; }
    const ComPtr<ID3D12CommandQueue>& GetCommandQueue() const{ return commandQueue_; }

private:
    ///////////////////////////////////////////////////
    //              リソース
    ///////////////////////////////////////////////////
    ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
    ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
    ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
};
