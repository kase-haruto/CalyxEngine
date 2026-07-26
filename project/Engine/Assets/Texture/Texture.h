#pragma once

#include <../DirectXTex/DirectXTex.h>

/* c++ */
#include <d3d12.h>
#include <string>
#include <wrl.h>

/*-----------------------------------------------------------------------------------------
 * Texture
 * - DirectXTexで読み込んだ画像とDirectX12テクスチャリソースを管理するクラス
 * - CPU画像のロード、GPUリソースへの転送、SRVの生成を担当する
 * - Descriptor Heap自体のライフタイムはDescriptorAllocatorへ委譲する
 *---------------------------------------------------------------------------------------*/
class Texture {
public:
	Texture() = default;
	Texture(const std::string& filePath, bool forceSrgb = true);
	~Texture();
	Texture(Texture&& other) noexcept;
	Texture& operator=(Texture&& other) noexcept;

	/// <summary>
	/// ロード
	/// </summary>
	/// <param name="device"></param>
	bool Load(ID3D12Device* device);

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
	void CreateShaderResourceView(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE destination) const;

	//--------- accessor -----------------------------------------------------
	// getter
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle() const { return srvHandleGPU_; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuSrvHandle() const { return srvHandleCPU_; }
	const DirectX::TexMetadata& GetMetaData();
	bool IsLoaded() const { return loaded_; }

private:
	std::string							   filePath_;
	bool								   forceSrgb_ = true;
	bool								   loaded_ = false; //< テクスチャ画像の読み込みに成功したか。
	DirectX::ScratchImage				   image_;
	DirectX::TexMetadata				   metadata_;
	Microsoft::WRL::ComPtr<ID3D12Resource> resource_;

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_ = {0};
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_ = {0};
};
