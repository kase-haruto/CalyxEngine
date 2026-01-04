#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cassert>
#include <stdint.h>


class DxFence{
    template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    DxFence() = default;
    ~DxFence();

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="device"></param>
    void Initialize(ComPtr<ID3D12Device> device);
    /// <summary>
    /// シグナル
    /// </summary>
    /// <param name="commandQueue"></param>
    void Signal(ComPtr<ID3D12CommandQueue> commandQueue);
    /// <summary>
    /// 待機
    /// </summary>
    void Wait();

private:
    ///////////////////////////////////////////////////
    //              リソース
    ///////////////////////////////////////////////////
    ComPtr<ID3D12Fence> fence_ = nullptr;
    HANDLE fenceEvent_ = nullptr;
    uint64_t fenceValue_ = 0;
};
