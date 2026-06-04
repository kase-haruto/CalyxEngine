#include "Texture.h"
/* ========================================================================
/* include space
/* ===================================================================== */

/* engine */
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>
#include <Engine/Foundation/Utility/Func/CxUtils.h>

/* lib */
#include <Engine/Foundation/Utility/Converter/ConvertString.h>

/* c++ */
#include <Engine/Foundation/Debug/CxAssert.h>
#include <d3dx12.h>

Texture::Texture(const std::string& filePath, bool forceSrgb) : filePath_(filePath), forceSrgb_(forceSrgb) {}

Texture::~Texture() {
	// リソースの解放処理
	resource_.Reset();
}

Texture::Texture(Texture&& other) noexcept
	: filePath_(std::move(other.filePath_)),
	forceSrgb_(other.forceSrgb_),
	image_(std::move(other.image_)),
	metadata_(std::move(other.metadata_)),
	resource_(std::move(other.resource_)),
	srvHandleCPU_(other.srvHandleCPU_),
	srvHandleGPU_(other.srvHandleGPU_) {
	other.srvHandleCPU_.ptr = 0;
	other.srvHandleGPU_.ptr = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
	if (this != &other) {
		filePath_ = std::move(other.filePath_);
		forceSrgb_ = other.forceSrgb_;
		image_ = std::move(other.image_);
		metadata_ = std::move(other.metadata_);
		resource_ = std::move(other.resource_);
		srvHandleCPU_ = other.srvHandleCPU_;
		srvHandleGPU_ = other.srvHandleGPU_;

		other.srvHandleCPU_.ptr = 0;
		other.srvHandleGPU_.ptr = 0;
	}
	return *this;
}

void Texture::Load([[maybe_unused]] ID3D12Device* device) {
	std::string fullPath = "Resources/Assets/" + filePath_;
	image_ = Cx::IO::LoadTextureImage(fullPath, forceSrgb_);
	metadata_ = image_.GetMetadata();
}

void Texture::Upload(ID3D12Device* device) {
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Width = UINT(metadata_.width);
	resourceDesc.Height = UINT(metadata_.height);
	resourceDesc.MipLevels = UINT16(metadata_.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metadata_.arraySize);
	resourceDesc.Format = metadata_.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	switch (metadata_.dimension) {
		case DirectX::TEX_DIMENSION_TEXTURE1D:
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			break;
		case DirectX::TEX_DIMENSION_TEXTURE2D:
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			break;
		case DirectX::TEX_DIMENSION_TEXTURE3D:
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			break;
		default:
			CX_CHECK(false && "Unsupported texture dimension", "Assertion failed");
	}

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource_)
	);
	CX_CHECK(SUCCEEDED(hr), "Assertion failed");

	for (size_t item = 0; item < metadata_.arraySize; ++item) {
		for (size_t mip = 0; mip < metadata_.mipLevels; ++mip) {
			const DirectX::Image* img = image_.GetImage(mip, item, 0);
			CX_CHECK(img != nullptr, "Assertion failed");

			UINT subresourceIndex = D3D12CalcSubresource(
				UINT(mip),
				UINT(item),
				0,
				UINT(metadata_.mipLevels),
				UINT(metadata_.arraySize)
			);

			hr = resource_->WriteToSubresource(
				subresourceIndex,
				nullptr,
				img->pixels,
				UINT(img->rowPitch),
				UINT(img->slicePitch)
			);
			CX_CHECK(SUCCEEDED(hr), "Assertion failed");
		}
	}
}


D3D12_SHADER_RESOURCE_VIEW_DESC BuildTextureSrvDesc(const DirectX::TexMetadata& metadata) {
	// SRV の設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (metadata.IsCubemap()){
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = UINT_MAX;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	} else{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = static_cast< UINT >(metadata.mipLevels);
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	}
	return srvDesc;
}

void Texture::CreateShaderResourceView(ID3D12Device* device){
	DescriptorHandle handle = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);
	srvHandleCPU_ = handle.cpu;
	srvHandleGPU_ = handle.gpu;

	// SRV を作成
	CreateShaderResourceView(device, srvHandleCPU_);
}

void Texture::CreateShaderResourceView(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE destination) const {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = BuildTextureSrvDesc(metadata_);
	device->CreateShaderResourceView(resource_.Get(), &srvDesc, destination);
}


const DirectX::TexMetadata& Texture::GetMetaData() {
	return metadata_;
}
