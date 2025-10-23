#pragma once
#include <Engine/Assets/Texture/Texture.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

/* c++ */
#include <d3d12.h>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <wrl.h>

// fwd
class ImGuiManager;

/// <summary>
/// テクスチャ管理
/// </summary>
class TextureManager {
public:
	static TextureManager* GetInstance();

	void Initialize(ImGuiManager* imgui);
	void Finalize();

	/// <summary>
	/// 初期化時ロード
	/// </summary>
	void StartUpLoad();

	/// <summary>
	/// テクスチャのロード
	/// </summary>
	/// <param name="filePath"></param>
	/// <returns></returns>
	D3D12_GPU_DESCRIPTOR_HANDLE LoadTexture(const std::string& filePath);
	D3D12_GPU_DESCRIPTOR_HANDLE LoadTexture(const Guid& guid);

	//--------- accessor -----------------------------------------------------
	// getter
	D3D12_GPU_DESCRIPTOR_HANDLE						GetSrvHandle(const std::string& textureName) const;
	const std::unordered_map<std::string, Texture>& GetLoadedTextures() const;
	D3D12_GPU_DESCRIPTOR_HANDLE						GetSrvHandle(const Guid& guid) const;
	bool											HasTexture(const Guid& guid) const;
	D3D12_GPU_DESCRIPTOR_HANDLE						GetEnvironmentTextureSrvHandle() const;

	// setter
	void SetEnvironmentTexture(const std::string& filePath);

private:
	TextureManager() = default;

	const struct AssetRecord* FindTextureRecord(const Guid& g) const;
	static std::string		  ToAssetsRelative(const std::filesystem::path& abs, const std::filesystem::path& root);

private:
	Microsoft::WRL::ComPtr<ID3D12Device> device_;
	ImGuiManager*						 imgui_				= nullptr;
	UINT								 descriptorSizeSrv_ = 0;

	std::unordered_map<std::string, Texture> textures_;
	std::unordered_map<Guid, std::string>	 guidToKey_;

	std::string environmentTextureName_;
};