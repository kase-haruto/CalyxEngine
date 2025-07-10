#pragma once

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

#include "DxBuffer.h"

template<typename T>
class DxConstantBuffer 
	: public DxBuffer<T>{
public:
	//===================================================================*/
	//                   public functions
	//===================================================================*/
	void Initialize(ComPtr<ID3D12Device> device, UINT elementCount = 1) override;

	void SetCommand(ComPtr<ID3D12GraphicsCommandList> cmdList, UINT rootParameterIndex)const override{
		cmdList->SetGraphicsRootConstantBufferView(rootParameterIndex, this->resource_->GetGPUVirtualAddress());
	}
};

template<typename T>
inline void DxConstantBuffer<T>::Initialize(ComPtr<ID3D12Device> device, UINT elementCount) {
	this->elementCount_ = elementCount;
	size_t byteSize = sizeof(T) * elementCount;

	// 256バイト境界にアライメントする！
	byteSize = (byteSize + 255) & ~255;

	this->CreateUploadResource(device, byteSize);
}