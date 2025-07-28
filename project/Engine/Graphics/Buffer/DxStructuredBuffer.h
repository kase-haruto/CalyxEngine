#pragma once

#include "DxBuffer.h"
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>

template<typename T>
class DxStructuredBuffer
	: public DxBuffer<T>{
public:
	//===================================================================*/
	//                   public functions
	//===================================================================*/
	void Initialize(ComPtr<ID3D12Device> device, UINT elementCount = 1) override;
	void SetCommand(ComPtr<ID3D12GraphicsCommandList> cmdList, UINT slot = 0);
	void CreateSrv(ComPtr<ID3D12Device> device);

	void ReleaseSrv();

	//--------- accessor -----------------------------------------------------
	// SRVハンドル取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const{ return gpuHandle_; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const{ return cpuHandle_; }
	// VertexBufferView を取得
	D3D12_VERTEX_BUFFER_VIEW GetView() const{ return vbView_; }

private:
	//===================================================================*/
	//                   private variables
	//===================================================================*/
	D3D12_VERTEX_BUFFER_VIEW vbView_ = {};

	// SRVハンドル
	DescriptorHandle srvHandle_;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle_ {};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle_ {};
};

template<typename T>
void DxStructuredBuffer<T>::Initialize(ComPtr<ID3D12Device> device, UINT elementCount){
	this->elementCount_ = elementCount;
	size_t byteSize = sizeof(T) * elementCount;
	this->CreateUploadResource(device, byteSize);

	// VertexBufferView 設定
	vbView_.BufferLocation = this->resource_->GetGPUVirtualAddress();
	vbView_.StrideInBytes = sizeof(T);
	vbView_.SizeInBytes = static_cast< UINT >(byteSize);
}

template<typename T>
void DxStructuredBuffer<T>::SetCommand(ComPtr<ID3D12GraphicsCommandList> cmdList, UINT slot){
	cmdList->IASetVertexBuffers(slot, 1, &vbView_);
}

template<typename T>
void DxStructuredBuffer<T>::CreateSrv(ComPtr<ID3D12Device> device){
	if (cpuHandle_.ptr != 0){
		// すでにSRVを作成済みなら再生成しない
		return;
	}

	srvHandle_ = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);
	cpuHandle_ = srvHandle_.cpu;
	gpuHandle_ = srvHandle_.gpu;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.NumElements = this->elementCount_;
	srvDesc.Buffer.StructureByteStride = sizeof(T);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	device->CreateShaderResourceView(this->resource_.Get(), &srvDesc, cpuHandle_);
}

template<typename T>
void DxStructuredBuffer<T>::ReleaseSrv(){
	if (srvHandle_.cpu.ptr != 0){
		DescriptorAllocator::Free(DescriptorUsage::CbvSrvUav, srvHandle_);
		srvHandle_ = {};
		cpuHandle_ = {};
		gpuHandle_ = {};
	}
}