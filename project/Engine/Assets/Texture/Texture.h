#pragma once

#include <../DirectXTex/DirectXTex.h>

/* c++ */
#include <d3d12.h>
#include <string>
#include <wrl.h>

/* ========================================================================
/*		テクスチャ
/* ===================================================================== */
class Texture {
public:
	Texture() = default;
	Texture(const std::string& filePath);
	~Texture();
	Texture(Texture&& other) noexcept;
	Texture& operator=(Texture&& other) noexcept;

	/// <summary>
	/// ロード
	/// </summary>
	/// <param name="device"></param>
	void Load(ID3D12Device* device);

	/// <summary>
	/// アップロード
	/// </summary>
	/// <param name="device"></param>
	void Upload(ID3D12Device* device);

	/// <summary>
	/// srv作成
	/// </summary>
	/// <param name="device"></param>
	void CreateShaderResourceView(ID3D12Device* device);

	//--------- accessor -----------------------------------------------------
	// getter
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle() const { return srvHandleGPU_; }
	const DirectX::TexMetadata& GetMetaData();

private:
	std::string							   filePath_;
	DirectX::ScratchImage				   image_;
	DirectX::TexMetadata				   metadata_;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource_;

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_ = {0};
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_ = {0};
};
