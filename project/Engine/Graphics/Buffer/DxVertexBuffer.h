#pragma once

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

#include "DxBuffer.h"

template <typename T>
class DxVertexBuffer
	: public DxBuffer<T> {
public:
	//===================================================================*/
	//                   public functions
	//===================================================================*/
	void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device,UINT elementCount = 1) override;

	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList,[[maybe_unused]] UINT rootParameterIndex = 0) const override {
		if(!this->resource_) {
			assert(false && "VertexBuffer resource is null. Initialize() might not have been called.");
			return;
		}

		// **入力アセンブラに頂点バッファを設定**
		cmdList->IASetVertexBuffers(0,1,&vertexBufferView_);
	}

	// ---- CPU 書き込みヘルパ（Upload ヒープ Initialize() 時のみ有効）----
	void Write(UINT index,const T& value) {
		assert(this->mappedPtr_ && "Write() requires mappedPtr_. Ensure DxBuffer maps upload resources.");
		assert(index < this->elementCount_);
		std::memcpy(static_cast<uint8_t*>(this->mappedPtr_) + sizeof(T) * index,&value,sizeof(T));
	}

	T* Data() {
		assert(this->mappedPtr_ && "Data() requires mappedPtr_. Ensure DxBuffer maps upload resources.");
		return reinterpret_cast<T*>(this->mappedPtr_);
	}

	// viewの取得 ===================================================================*/
	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBufferView_; }

private:
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ = {};
};

template <typename T>
inline void DxVertexBuffer<T>::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device,UINT elementCount) {
	this->elementCount_ = elementCount;
	size_t byteSize     = sizeof(T) * elementCount;
	this->CreateUploadResource(device,byteSize);

	vertexBufferView_.BufferLocation = this->resource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes    = static_cast<UINT>(byteSize);
	vertexBufferView_.StrideInBytes  = sizeof(T);
}