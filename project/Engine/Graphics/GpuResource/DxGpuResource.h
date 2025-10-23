#pragma once

#include <d3d12.h>
#include <optional>
#include <string>
#include <wrl.h>

/* ========================================================================
/* gpuリソース
/* ===================================================================== */
class DxGpuResource {
public:
	DxGpuResource()	 = default;
	~DxGpuResource() = default;

	// リソースの生成
	void InitializeAsRenderTarget(ID3D12Device*				  device,
								  uint32_t					  width,
								  uint32_t					  height,
								  DXGI_FORMAT				  format,
								  std::optional<std::wstring> name = std::nullopt);
	/// <summary>
	/// srv作成
	/// </summary>
	/// <param name="device"></param>
	void CreateSRV(ID3D12Device* device);

	/// <summary>
	/// rtv作成
	/// </summary>
	/// <param name="device"></param>
	/// <param name="handle"></param>
	void CreateRTV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle);

	/// <summary>
	/// リソース遷移
	/// </summary>
	/// <param name="cmdList"></param>
	/// <param name="newState"></param>
	void Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);

	/// <summary>
	/// 現在のステート設定
	/// </summary>
	/// <param name="state"></param>
	void SetCurrentState(D3D12_RESOURCE_STATES state);

	// アクセサ
	ID3D12Resource*				Get() const { return resource_.Get(); }
	D3D12_RESOURCE_STATES		GetCurrentState() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCpuHandle() const { return cpuSrvHandle_; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGpuHandle() const { return gpuSrvHandle_; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCpuHandle() const { return cpuRtvHandle_; }

private:
	D3D12_RESOURCE_STATES				   currentState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
	D3D12_CPU_DESCRIPTOR_HANDLE			   cpuSrvHandle_{};
	D3D12_GPU_DESCRIPTOR_HANDLE			   gpuSrvHandle_{};
	D3D12_CPU_DESCRIPTOR_HANDLE			   cpuRtvHandle_{};
};
