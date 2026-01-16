#pragma once

#include <cassert>
#include <d3d12.h>
#include <stdint.h>
#include <wrl.h>

/*-----------------------------------------------------------------------------------------
 * DxFence class
 * - DirectXフェンス同期を管理するクラス
 * - シグナル/待機によるGPU同期を提供する
 *---------------------------------------------------------------------------------------*/
class DxFence {
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

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
	ComPtr<ID3D12Fence> fence_		= nullptr;
	HANDLE				fenceEvent_ = nullptr;
	uint64_t			fenceValue_ = 0;
};
