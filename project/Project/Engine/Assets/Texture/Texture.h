#pragma once


#include <../DirectXTex/DirectXTex.h>

/* c++ */
#include <d3d12.h>
#include <wrl.h>
#include <string>

class Texture{
public:
	Texture() = default;
	Texture(const std::string& filePath);
	~Texture();

	// ムーブコンストラクタとムーブ代入演算子の宣言
	Texture(Texture&& other) noexcept;
	Texture& operator=(Texture&& other) noexcept;

	void Load(ID3D12Device* device);
	void Upload(ID3D12Device* device);
	void CreateShaderResourceView(ID3D12Device* device);

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

