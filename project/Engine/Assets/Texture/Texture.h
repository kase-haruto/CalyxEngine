#pragma once

// exterunal
#include <externals/DirectXTex/DirectXTex.h>

/* c++ */
#include <d3d12.h>
#include <wrl.h>
#include <string>

/// <summary>
/// テクスチャクラス
/// </summary>
class Texture{
public:
	Texture() = default;
	Texture(const std::string& filePath);
	~Texture();
	Texture(Texture&& other) noexcept;
	Texture& operator=(Texture&& other) noexcept;

	// ロード
	void Load(ID3D12Device* device);
	// アップロード
	void Upload(ID3D12Device* device);
	// srv作成
	void CreateShaderResourceView(ID3D12Device* device);

	//--------- accessor -----------------------------------------------------
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle() const{ return srvHandleGPU_; }
	const DirectX::TexMetadata& GetMetaData();

private:
	std::string filePath_;
	DirectX::ScratchImage image_;
	DirectX::TexMetadata metadata_;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource_;

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_ = {0};
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_ = {0};

};

