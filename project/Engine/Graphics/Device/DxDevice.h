#pragma once
#include <cassert>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>

/// <summary>
/// デバイス
/// </summary>
class DxDevice {
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	DxDevice() = default;
	~DxDevice();

	void						 Initialize();
	const ComPtr<ID3D12Device>&	 GetDevice() const { return device_; }
	const ComPtr<IDXGIFactory7>& GetDXGIFactory() const { return dxgiFactory_; }

private:
	/// <summary>
	/// デバッグレイヤーの設定
	/// </summary>
	void SetupDebugLayer();
	/// <summary>
	/// デバイスの作成
	/// </summary>
	void CreateDXGIDevice();

private:
	///////////////////////////////////////////////////
	//              リソース
	///////////////////////////////////////////////////
	ComPtr<ID3D12Device>  device_		   = nullptr;
	ComPtr<IDXGIAdapter4> adapter_		   = nullptr;
	ComPtr<IDXGIFactory7> dxgiFactory_	   = nullptr;
	ComPtr<ID3D12Debug1>  debugController_ = nullptr;
};
