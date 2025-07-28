#include "TextureManager.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
/* engine */
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Application/UI/GUI/ImGuiManager.h>

/* c++ */
#include <cassert>

TextureManager* TextureManager::GetInstance(){
	static TextureManager instance;
	return &instance;
}


void TextureManager::StartUpLoad(){
	LoadTexture("Textures/uvChecker.png");
	LoadTexture("Textures/MonsterBall.png");
	LoadTexture("Textures/flower.png");
	LoadTexture("Textures/smoke.png");
	LoadTexture("Textures/redCircle.png");
	LoadTexture("Textures/fieldTile.png");
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandle(const std::string& textureName) const{
	auto it = textures_.find(textureName);
	if (it != textures_.end()){
		return it->second.GetSrvHandle();
	}
	return {};
}

const std::unordered_map<std::string, Texture>& TextureManager::GetLoadedTextures() const{
	return textures_;
}

void TextureManager::SetEnvironmentTexture(const std::string& filePath){
	// 環境テクスチャの読み込み
	environmentTextureName_ = filePath;
	if (textures_.find(filePath) == textures_.end()){
		Texture texture(filePath);
		texture.Load(device_.Get());
		texture.Upload(device_.Get());
		texture.CreateShaderResourceView(device_.Get());
		textures_[filePath] = std::move(texture);
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetEnvironmentTextureSrvHandle() const{
	auto it = textures_.find(environmentTextureName_);
	if (it != textures_.end()){
		return it->second.GetSrvHandle();
	}
	return {};
}

void TextureManager::Initialize(ImGuiManager* imgui){
	device_ = GraphicsGroup::GetInstance()->GetDevice();
	imgui_ = imgui;
	descriptorSizeSrv_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void TextureManager::Finalize(){
	for (auto& texture : textures_){
		texture.second.~Texture();
	}
	device_.Reset();
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::LoadTexture(const std::string& filePath){
	// テクスチャが既にロードされているか確認
	if (textures_.find(filePath) != textures_.end()){
		return textures_[filePath].GetSrvHandle();
	}

	// 新しいテクスチャをロード
	Texture texture(filePath);
	texture.Load(device_.Get());
	texture.Upload(device_.Get());

	// シェーダリソースビューを作成
	texture.CreateShaderResourceView(device_.Get());

	// SRVハンドルを取得し、マップに追加
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = texture.GetSrvHandle();
	textures_[filePath] = std::move(texture);

	return gpuHandle;
}

