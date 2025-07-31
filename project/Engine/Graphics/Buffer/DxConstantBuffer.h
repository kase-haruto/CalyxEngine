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
	void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT elementCount = 1) override;

	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList, UINT rootParameterIndex)const override{
		cmdList->SetGraphicsRootConstantBufferView(rootParameterIndex, this->resource_->GetGPUVirtualAddress());
	}

	void SetCompute(ID3D12GraphicsCommandList* cmd, UINT rootSlot) const;
};

template<typename T>
inline void DxConstantBuffer<T>::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT elementCount) {
	this->elementCount_ = elementCount;
	size_t byteSize = sizeof(T) * elementCount;

	// 256バイト境界にアライメントする！
	byteSize = (byteSize + 255) & ~255;

	this->CreateUploadResource(device, byteSize);
}

template<typename T>
inline void DxConstantBuffer<T>::SetCompute(ID3D12GraphicsCommandList* cmd, UINT slot) const{
	cmd->SetComputeRootConstantBufferView(slot, this->resource_->GetGPUVirtualAddress());
}
