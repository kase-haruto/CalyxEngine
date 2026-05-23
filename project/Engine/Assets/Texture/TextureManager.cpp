#include "TextureManager.h"
/* engine */
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Application/UI/GUI/ImGuiManager.h>
#include <Engine/Assets/Database/AssetDatabase.h>
/* c++ */
#include <Engine/Foundation/Debug/CxAssert.h>
#include <system_error>

using Microsoft::WRL::ComPtr;

void TextureManager::StartUpLoad() {
}

void TextureManager::Initialize(ImGuiManager* imgui) {
	device_ = GraphicsGroup::GetInstance()->GetDevice();
	imgui_ = imgui;
	descriptorSizeSrv_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void TextureManager::Finalize() {
	for (auto& kv : textures_) {
		kv.second.~Texture();
	}
	device_.Reset();
}

/* ============ 既存：文字列キー ============ */
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::LoadTexture(const std::string& filePath) {
	// 既にロード済み？
	if (auto it = textures_.find(filePath); it != textures_.end()) {
		return it->second.GetSrvHandle();
	}

	// 新規ロード
	Texture texture(filePath);
	texture.Load(device_.Get());
	texture.Upload(device_.Get());
	texture.CreateShaderResourceView(device_.Get());

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = texture.GetSrvHandle();
	textures_[filePath] = std::move(texture);

	if (auto* db = AssetDatabase::GetInstance()) {
		std::error_code ec;
		auto root = db->GetRoot();
		auto abs = std::filesystem::weakly_canonical(root / std::filesystem::path(filePath), ec);
		if (!ec) {
			// DB を走査して一致するレコードを探す（最小実装）
			for (auto* r : db->GetView()) {
				if (!r || r->type != AssetType::Texture) continue;
				if (std::filesystem::weakly_canonical(r->sourcePath, ec) == abs) {
					guidToKey_[r->guid] = filePath; // GUID → 文字列キー
					break;
				}
			}
		}
	}

	return gpuHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandle(const std::string& textureName) const {
	auto it = textures_.find(textureName);
	if (it != textures_.end()) {
		return it->second.GetSrvHandle();
	}
	return {};
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::GetCpuSrvHandle(const std::string& textureName) const {
	auto it = textures_.find(textureName);
	if(it != textures_.end()) {
		return it->second.GetCpuSrvHandle();
	}
	const_cast<TextureManager*>(this)->LoadTexture(textureName);
	if(auto loaded = textures_.find(textureName); loaded != textures_.end()) {
		return loaded->second.GetCpuSrvHandle();
	}
	return {};
}

bool TextureManager::WriteSrvTo(const std::string& textureName, D3D12_CPU_DESCRIPTOR_HANDLE destination) const {
	if(!destination.ptr) return false;

	auto it = textures_.find(textureName);
	if(it == textures_.end()) {
		const_cast<TextureManager*>(this)->LoadTexture(textureName);
		it = textures_.find(textureName);
	}
	if(it == textures_.end()) return false;

	it->second.CreateShaderResourceView(device_.Get(), destination);
	return true;
}

const std::unordered_map<std::string, Texture>& TextureManager::GetLoadedTextures() const {
	return textures_;
}

// GUID→AssetRecord（テクスチャ）を探す（最小実装：線形探索）
const AssetRecord* TextureManager::FindTextureRecord(const Guid& g) const {
	if (!g.isValid()) return nullptr;
	auto* db = AssetDatabase::GetInstance();
	if (!db) return nullptr;
	for (auto* r : db->GetView()) {
		if (!r) continue;
		if (r->type == AssetType::Texture && r->guid == g) return r;
	}
	return nullptr;
}

std::string TextureManager::ToAssetsRelative(const std::filesystem::path& abs,
											 const std::filesystem::path& root) {
	std::error_code ec;
	auto rel = std::filesystem::relative(abs, root, ec);
	return (ec ? abs : rel).generic_string();
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::LoadTexture(const Guid& guid) {
	if (!guid.isValid()) return {};

	// 既に GUID が紐づいていれば、そのキーで返す
	if (auto it = guidToKey_.find(guid); it != guidToKey_.end()) {
		return LoadTexture(it->second);
	}

	// DB から GUID→パス解決
	const AssetRecord* rec = FindTextureRecord(guid);
	if (!rec) return {};

	// Assets ルート相対キーに揃える（既存の LoadTexture(string) に渡す）
	auto* db = AssetDatabase::GetInstance();
	std::string key = ToAssetsRelative(rec->sourcePath, db->GetRoot());

	auto h = LoadTexture(key);
	if (h.ptr) guidToKey_.emplace(guid, key);
	return h;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandle(const Guid& guid) const {
	if (!guid.isValid()) return {};
	// すでにキー判明＆ロード済みならそのまま
	if (auto it = guidToKey_.find(guid); it != guidToKey_.end()) {
		if (auto it2 = textures_.find(it->second); it2 != textures_.end()) {
			return it2->second.GetSrvHandle();
		}
	}
	// const だがロードしたいケース用に const_cast（薄いラッパなので許容）
	return const_cast<TextureManager*>(this)->LoadTexture(guid);
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::GetCpuSrvHandle(const Guid& guid) const {
	if(!guid.isValid()) return {};
	if(auto it = guidToKey_.find(guid); it != guidToKey_.end()) {
		if(auto it2 = textures_.find(it->second); it2 != textures_.end()) {
			return it2->second.GetCpuSrvHandle();
		}
	}
	const_cast<TextureManager*>(this)->LoadTexture(guid);
	if(auto it = guidToKey_.find(guid); it != guidToKey_.end()) {
		if(auto it2 = textures_.find(it->second); it2 != textures_.end()) {
			return it2->second.GetCpuSrvHandle();
		}
	}
	return {};
}

bool TextureManager::WriteSrvTo(const Guid& guid, D3D12_CPU_DESCRIPTOR_HANDLE destination) const {
	if(!guid.isValid() || !destination.ptr) return false;

	if(auto it = guidToKey_.find(guid); it != guidToKey_.end()) {
		return WriteSrvTo(it->second, destination);
	}

	const_cast<TextureManager*>(this)->LoadTexture(guid);
	if(auto it = guidToKey_.find(guid); it != guidToKey_.end()) {
		return WriteSrvTo(it->second, destination);
	}
	return false;
}

bool TextureManager::HasTexture(const Guid& guid) const {
	if (!guid.isValid()) return false;
	auto it = guidToKey_.find(guid);
	if (it == guidToKey_.end()) return false;
	return textures_.find(it->second) != textures_.end();
}

/* ============ 環境マップ（既存） ============ */
void TextureManager::SetEnvironmentTexture(const std::string& filePath) {
	environmentTextureName_ = filePath;
	if (textures_.find(filePath) == textures_.end()) {
		Texture texture(filePath);
		texture.Load(device_.Get());
		texture.Upload(device_.Get());
		texture.CreateShaderResourceView(device_.Get());
		textures_[filePath] = std::move(texture);
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetEnvironmentTextureSrvHandle() const {
	auto it = textures_.find(environmentTextureName_);
	if (it != textures_.end()) {
		return it->second.GetSrvHandle();
	}
	return {};
}
