#pragma once

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

#include "DxBuffer.h"

template<typename T>
class DxIndexBuffer
	: public DxBuffer<T>{
public:
	//===================================================================*/
	//                   public functions
	//===================================================================*/
	void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT elementCount = 1) override;

	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList, [[maybe_unused]] UINT rootParameterIndex = 0)const override{
		cmdList->IASetIndexBuffer(&indexBufferView_);
	}

	// ---- CPU 書き込みヘルパ（Upload ヒープ Initialize() 時のみ有効）----
	void Write(UINT index, const T& value) {
		assert(this->mappedPtr_ && "Write() requires mappedPtr_. Ensure DxBuffer maps upload resources.");
		assert(index < this->elementCount_);
		std::memcpy(static_cast<uint8_t*>(this->mappedPtr_) + sizeof(T) * index, &value, sizeof(T));
	}

	T* Data() {
		assert(this->mappedPtr_ && "Data() requires mappedPtr_. Ensure DxBuffer maps upload resources.");
		return reinterpret_cast<T*>(this->mappedPtr_);
	}

	
	// viewの取得 ===================================================================*/
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const{ return indexBufferView_; }

private:
	D3D12_INDEX_BUFFER_VIEW indexBufferView_ = {};
};

template<typename T>
inline void DxIndexBuffer<T>::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT elementCount){
	this->elementCount_ = elementCount;
	size_t byteSize = sizeof(T) * elementCount;
	this->CreateUploadResource(device, byteSize);

	indexBufferView_.BufferLocation = this->resource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = static_cast< UINT >(byteSize);
	indexBufferView_.Format = (std::is_same_v<T, uint16_t>) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
}
