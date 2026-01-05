#pragma once

#include "DxBuffer.h"
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>
#include <cstring>
#include <d3dx12.h>

template <typename T>
class DxStructuredBuffer : public DxBuffer<T> {
public:
	//===================================================================*/
	//                   public functions
	//===================================================================*/
	void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT elementCount = 1) override;
	void InitializeAsRW(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT elementCount);

	// SRV/UAV 作成（範囲指定可：省略時は全域）
	void CreateSrv(ID3D12Device* device, UINT firstElement = 0, UINT numElements = UINT_MAX);
	void CreateUav(ID3D12Device* device, UINT firstElement = 0, UINT numElements = UINT_MAX);

	// 既存 SRV/UAV を解放（ディスクリプタ返却）
	void ReleaseSrv();
	void ReleaseUav();

	// ---- CPU 書き込みヘルパ（Uploadヒープ Initialize() 時のみ有効）----
	void Write(UINT index, const T& value);
	T*	 Data(); // 連続領域に一括 memcpy したい場合

	// ---- 管理系ヘルパ ----
	UINT GetElementCount() const { return this->elementCount_; }
	bool IsValid() const { return this->resource_ != nullptr; }
	void Reset();															 // リソース/ハンドル全解放
	void Resize(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT newCount); // 再確保（Upload）

	// ---- アクセサ ----
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuSrvHandle() const { return srvHandle_.gpu; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuSrvHandle() const { return srvHandle_.cpu; }

	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuUavHandle() const { return uavHandle_.gpu; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuUavHandle() const { return uavHandle_.cpu; }

	[[deprecated("StructuredBuffer is not a vertex buffer. Bind via SetGraphicsRootDescriptorTable.")]]
	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList, UINT slot = 0) {
		cmdList->IASetVertexBuffers(slot, 1, &vbView_);
	}

private:
	//===================================================================*/
	//                   private variables
	//===================================================================*/
	// 互換維持（未使用推奨）
	D3D12_VERTEX_BUFFER_VIEW vbView_ = {};

	DescriptorHandle srvHandle_{};
	DescriptorHandle uavHandle_{};
};

// ------------------------ Implementation ------------------------ //

template <typename T>
void DxStructuredBuffer<T>::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT elementCount) {
	this->elementCount_	  = elementCount;
	const size_t byteSize = sizeof(T) * elementCount;

	// Upload ヒープ（CPU 書き込み可）
	this->CreateUploadResource(device, byteSize);

	// 互換
	vbView_.BufferLocation = this->resource_->GetGPUVirtualAddress();
	vbView_.StrideInBytes  = sizeof(T);
	vbView_.SizeInBytes	   = static_cast<UINT>(byteSize);
}

template <typename T>
void DxStructuredBuffer<T>::InitializeAsRW(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT elementCount) {
	this->elementCount_	  = elementCount;
	const size_t byteSize = sizeof(T) * elementCount;

	// DEFAULT Heap + UAV
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC	resDesc = CD3DX12_RESOURCE_DESC::Buffer(
		  byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	[[maybe_unused]] HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&this->resource_));
	assert(SUCCEEDED(hr) && "StructuredBuffer (RW) creation failed.");

	// DEFAULT ヒープなので Map 不可
	this->mappedPtr_ = nullptr;
}

template <typename T>
void DxStructuredBuffer<T>::CreateSrv(ID3D12Device* device, UINT firstElement, UINT numElements) {
	assert(this->resource_ && "resource_ is null before CreateSrv");
	if(numElements == UINT_MAX) numElements = this->elementCount_ - firstElement;

	// 既存ディスクリプタがある場合は再利用（同じCPUハンドルに作り直す）
	if(srvHandle_.cpu.ptr == 0) {
		srvHandle_ = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format							= DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement				= firstElement;
	srvDesc.Buffer.NumElements				= numElements;
	srvDesc.Buffer.StructureByteStride		= sizeof(T);
	srvDesc.Buffer.Flags					= D3D12_BUFFER_SRV_FLAG_NONE;

	device->CreateShaderResourceView(this->resource_.Get(), &srvDesc, srvHandle_.cpu);
}

template <typename T>
void DxStructuredBuffer<T>::CreateUav(ID3D12Device* device, UINT firstElement, UINT numElements) {
	assert(this->resource_ && "resource_ is null before CreateUav");
	if(numElements == UINT_MAX) numElements = this->elementCount_ - firstElement;

	if(uavHandle_.cpu.ptr == 0) {
		uavHandle_ = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension					 = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format							 = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.FirstElement				 = firstElement;
	uavDesc.Buffer.NumElements				 = numElements;
	uavDesc.Buffer.StructureByteStride		 = sizeof(T);
	uavDesc.Buffer.Flags					 = D3D12_BUFFER_UAV_FLAG_NONE;

	device->CreateUnorderedAccessView(this->resource_.Get(), nullptr, &uavDesc, uavHandle_.cpu);
}

template <typename T>
void DxStructuredBuffer<T>::ReleaseSrv() {
	if(srvHandle_.cpu.ptr != 0) {
		DescriptorAllocator::Free(DescriptorUsage::CbvSrvUav, srvHandle_);
		srvHandle_ = {};
	}
}

template <typename T>
void DxStructuredBuffer<T>::ReleaseUav() {
	if(uavHandle_.cpu.ptr != 0) {
		DescriptorAllocator::Free(DescriptorUsage::CbvSrvUav, uavHandle_);
		uavHandle_ = {};
	}
}

template <typename T>
void DxStructuredBuffer<T>::Write(UINT index, const T& value) {
	assert(this->mappedPtr_ && "Write() requires Initialize(upload) resource.");
	assert(index < this->elementCount_);
	std::memcpy(static_cast<uint8_t*>(this->mappedPtr_) + sizeof(T) * index, &value, sizeof(T));
}

template <typename T>
T* DxStructuredBuffer<T>::Data() {
	return reinterpret_cast<T*>(this->mappedPtr_);
}

template <typename T>
void DxStructuredBuffer<T>::Reset() {
	ReleaseSrv();
	ReleaseUav();
	this->resource_.Reset();
	this->mappedPtr_	= nullptr;
	this->elementCount_ = 0;
	vbView_				= {};
}

template <typename T>
void DxStructuredBuffer<T>::Resize(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT newCount) {
	if(newCount == this->elementCount_) return;
	Reset();
	Initialize(device, newCount); // Uploadヒープ再確保
}