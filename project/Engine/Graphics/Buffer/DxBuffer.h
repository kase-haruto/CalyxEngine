#pragma once

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// lib
#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <vector>
#include <cassert>


/////////////////////////////////////////////////////////////////////////////////////////
//	汎用bufferクラス
/////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
class DxBuffer{
public:
	//===================================================================*/
	//                   public functions
	//===================================================================*/
	DxBuffer() = default;
	virtual ~DxBuffer() = default;

	virtual void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT elementCount = 1) = 0;
	virtual void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>cmdList, UINT rootParameterIndex)const;
	virtual void TransferData(const T& data);
	virtual void TransferData(const T* data, UINT count);
	void TransferVectorData(const std::vector<T>& data);

	bool IsInitialized() const{
		return resource_ != nullptr;
	}

	// リソースの取得 ===================================================================*/
	Microsoft::WRL::ComPtr<ID3D12Resource> GetResource() const{ return resource_; }
	UINT GetRootParameterIndex() const{ return rootParameterIndex_; }

protected:
	//===================================================================*/
	//                   protected functions
	//===================================================================*/
	void CreateUploadResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t byteSize);

protected:
	//===================================================================*/
	//                   protected variables
	//===================================================================*/
	Microsoft::WRL::ComPtr<ID3D12Resource> resource_ = nullptr;
	UINT8* mappedPtr_ = nullptr;
	UINT elementCount_ = 0;
	UINT rootParameterIndex_ = 0;
};

template<typename T>
inline void DxBuffer<T>::SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList, UINT rootParameterIndex)const{
	if (!resource_){
		assert(false && "DxBuffer: resource is null.");
		return;
	}
	cmdList->SetGraphicsRootConstantBufferView(rootParameterIndex, resource_->GetGPUVirtualAddress());
}

/////////////////////////////////////////////////////////////////////////////////////////
//	データ転送
/////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
void DxBuffer<T>::TransferData(const T& data) {
	assert(mappedPtr_ && "Resource is not mapped!");
	std::memcpy(mappedPtr_, &data, sizeof(T));
}

template<typename T>
inline void DxBuffer<T>::TransferData(const T* data, UINT count){
	assert(mappedPtr_ && "Resource is not mapped!");
	std::memcpy(mappedPtr_, data, sizeof(T) * count);
}

template<typename T>
inline void DxBuffer<T>::TransferVectorData(const std::vector<T>& data){
	assert(!data.empty());
	this->TransferData(data.data(), static_cast< UINT >(data.size()));
}

/////////////////////////////////////////////////////////////////////////////////////////
//	bufferの作成
/////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
void DxBuffer<T>::CreateUploadResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t byteSize){
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = byteSize;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource_)
	);

	assert(SUCCEEDED(hr) && "Failed to create Upload Resource.");

	// マップ
	hr = resource_->Map(0, nullptr, reinterpret_cast< void** >(&mappedPtr_));
	assert(SUCCEEDED(hr) && "Failed to Map Upload Resource.");
}