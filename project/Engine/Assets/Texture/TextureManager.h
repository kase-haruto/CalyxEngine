#pragma once
#include <Engine/Assets/Texture/Texture.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

/* c++ */
#include <unordered_map>
#include <string>
#include <d3d12.h>
#include <wrl.h>
#include <filesystem>

class ImGuiManager;

class TextureManager {
public:
	static TextureManager* GetInstance();

	void Initialize(ImGuiManager* imgui);
	void Finalize();

	// 既存API（文字列キー）
	D3D12_GPU_DESCRIPTOR_HANDLE LoadTexture(const std::string& filePath);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle(const std::string& textureName) const;
	const std::unordered_map<std::string, Texture>& GetLoadedTextures() const;

	// ========= ここから GUID 対応  =========
	D3D12_GPU_DESCRIPTOR_HANDLE LoadTexture(const Guid& guid);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle(const Guid& guid) const;
	bool HasTexture(const Guid& guid) const;
	void SetEnvironmentTexture(const std::string& filePath);
	D3D12_GPU_DESCRIPTOR_HANDLE GetEnvironmentTextureSrvHandle() const;

	void StartUpLoad();

private:
	TextureManager() = default;

	const struct AssetRecord* FindTextureRecord(const Guid& g) const;
	static std::string ToAssetsRelative(const std::filesystem::path& abs, const std::filesystem::path& root);

private:
	Microsoft::WRL::ComPtr<ID3D12Device> device_;
	ImGuiManager* imgui_ = nullptr;
	UINT descriptorSizeSrv_ = 0;

	std::unordered_map<std::string, Texture> textures_;
	std::unordered_map<Guid, std::string> guidToKey_;

	std::string environmentTextureName_;
};