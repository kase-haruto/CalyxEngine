#pragma once

// std
#include <d3d12.h>
#include <optional>
#include <string>
#include <wrl.h>

/* ========================================================================
/*		gpu リソース
/* ===================================================================== */
class DxGpuResource {
public:
	//===================================================================*/
	//				public methods
	//===================================================================*/
	DxGpuResource()	 = default;
	~DxGpuResource() = default;

	/// <summary>
	/// リソース作成
	/// </summary>
	/// <param name="device"></param>
	/// <param name="width"></param>
	/// <param name="height"></param>
	/// <param name="format"></param>
	/// <param name="name"></param>
	void InitializeAsRenderTarget(ID3D12Device*				  device,
								  uint32_t					  width,
								  uint32_t					  height,
								  DXGI_FORMAT				  format,
								  std::optional<std::wstring> name = std::nullopt);

	void InitializeAsDepthStencil(ID3D12Device*				  device,
								  uint32_t					  width,
								  uint32_t					  height,
								  DXGI_FORMAT				  format,
								  std::optional<std::wstring> name = std::nullopt);

	/// <summary>
	/// srvの作成
	/// </summary>
	/// <param name="device"></param>
	void CreateSRV(ID3D12Device* device);

	/// <summary>
	/// rtv生成
	/// </summary>
	/// <param name="device"></param>
	/// <param name="handle"></param>
	void CreateRTV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle);

	void CreateDSV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle);

	/// <summary>
	/// srv更新
	/// </summary>
	/// <param name="device"></param>
	void UpdateSRV(ID3D12Device* device);

	/// <summary>
	/// transition
	/// </summary>
	/// <param name="cmdList"></param>
	/// <param name="newState"></param>
	void Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);

	//--------- accessor -----------------------------------------------------
	ID3D12Resource* Get() const { return resource_.Get(); }

	D3D12_RESOURCE_STATES		GetCurrentState() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCpuHandle() const { return cpuSrvHandle_; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGpuHandle() const { return gpuSrvHandle_; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCpuHandle() const { return cpuRtvHandle_; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCpuHandle() const { return cpuDsvHandle_; }

	void SetCurrentState(D3D12_RESOURCE_STATES state);

private:
	D3D12_RESOURCE_STATES				   currentState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
	D3D12_CPU_DESCRIPTOR_HANDLE			   cpuSrvHandle_{};
	D3D12_GPU_DESCRIPTOR_HANDLE			   gpuSrvHandle_{};
	D3D12_CPU_DESCRIPTOR_HANDLE			   cpuRtvHandle_{};
	D3D12_CPU_DESCRIPTOR_HANDLE			   cpuDsvHandle_{};
};
