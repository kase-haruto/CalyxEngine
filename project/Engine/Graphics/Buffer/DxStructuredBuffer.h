#pragma once

#include "DxBuffer.h"
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>
#include <d3dx12.h>

template<typename T>
class DxStructuredBuffer 
	: public DxBuffer<T>{
public:
	//===================================================================*/
	//                   public functions
	//===================================================================*/
	void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT elementCount = 1) override;
	void InitializeAsRW(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT elementCount);
	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList, UINT slot = 0);

	void CreateSrv(ID3D12Device* device);
	void CreateUav(ID3D12Device* device);

	void ReleaseSrv();
	void ReleaseUav();

	//--------- accessor -----------------------------------------------------
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuSrvHandle() const{ return srvHandle_.gpu; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuSrvHandle() const{ return srvHandle_.cpu; }

	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuUavHandle() const{ return uavHandle_.gpu; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuUavHandle() const{ return uavHandle_.cpu; }

	D3D12_VERTEX_BUFFER_VIEW GetView() const{ return vbView_; }

private:
	//===================================================================*/
	//                   private variables
	//===================================================================*/
	D3D12_VERTEX_BUFFER_VIEW vbView_ = {};

	DescriptorHandle srvHandle_ {};
	DescriptorHandle uavHandle_ {};
};

template<typename T>
void DxStructuredBuffer<T>::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT elementCount){
	this->elementCount_ = elementCount;
	size_t byteSize = sizeof(T) * elementCount;

	// CreateUploadResource → ALLOW_UNORDERED_ACCESS を使いたいなら別関数を用意すべき
	this->CreateUploadResource(device, byteSize);

	vbView_.BufferLocation = this->resource_->GetGPUVirtualAddress();
	vbView_.StrideInBytes = sizeof(T);
	vbView_.SizeInBytes = static_cast< UINT >(byteSize);
}

template<typename T>
void DxStructuredBuffer<T>::SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList, UINT slot){
	cmdList->IASetVertexBuffers(slot, 1, &vbView_);
}

template<typename T>
void DxStructuredBuffer<T>::CreateSrv(ID3D12Device* device){
	if (srvHandle_.cpu.ptr != 0) return;
	assert(this->resource_ && "resource_ is null before CreateSrv");
	srvHandle_ = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = this->elementCount_;
	srvDesc.Buffer.StructureByteStride = sizeof(T);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	device->CreateShaderResourceView(this->resource_.Get(), &srvDesc, srvHandle_.cpu);
}

template<typename T>
void DxStructuredBuffer<T>::CreateUav(ID3D12Device* device){
	if (uavHandle_.cpu.ptr != 0) return;

	uavHandle_ = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = this->elementCount_;
	uavDesc.Buffer.StructureByteStride = sizeof(T);
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	device->CreateUnorderedAccessView(this->resource_.Get(), nullptr, &uavDesc, uavHandle_.cpu);
}

template<typename T>
void DxStructuredBuffer<T>::ReleaseSrv(){
	if (srvHandle_.cpu.ptr != 0){
		DescriptorAllocator::Free(DescriptorUsage::CbvSrvUav, srvHandle_);
		srvHandle_ = {};
	}
}

template<typename T>
void DxStructuredBuffer<T>::ReleaseUav(){
	if (uavHandle_.cpu.ptr != 0){
		DescriptorAllocator::Free(DescriptorUsage::CbvSrvUav, uavHandle_);
		uavHandle_ = {};
	}
}

template<typename T>
void DxStructuredBuffer<T>::InitializeAsRW(
	Microsoft::WRL::ComPtr<ID3D12Device> device,
	UINT elementCount){
	this->elementCount_ = elementCount;
	const size_t byteSize = sizeof(T) * elementCount;

	// DEFAULT Heap + UAV
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC   resDesc =
		CD3DX12_RESOURCE_DESC::Buffer(byteSize,
									  D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	[[maybe_unused]] HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&this->resource_));
	assert(SUCCEEDED(hr) && "StructuredBuffer (RW) creation failed.");

	// ※ DEFAULT ヒープなので Map は行わず (mappedPtr_ は nullptr)
	this->mappedPtr_ = nullptr;
}