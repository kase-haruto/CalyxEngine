#pragma once

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

#include "DxBuffer.h"
#include <Engine/Foundation/Debug/CxAssert.h>

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
			CX_CHECK(false && "VertexBuffer resource is null. Initialize() might not have been called.", "Assertion failed");
			return;
		}

		// **入力アセンブラに頂点バッファを設定**
		cmdList->IASetVertexBuffers(0,1,&vertexBufferView_);
	}

	// ---- CPU 書き込みヘルパ（Upload ヒープ Initialize() 時のみ有効）----
	void Write(UINT index,const T& value) {
		CX_CHECK(this->mappedPtr_ && "Write() requires mappedPtr_. Ensure DxBuffer maps upload resources.", "Assertion failed");
		CX_CHECK(index < this->elementCount_, "Assertion failed");
		std::memcpy(static_cast<uint8_t*>(this->mappedPtr_) + sizeof(T) * index,&value,sizeof(T));
	}

	T* Data() {
		CX_CHECK(this->mappedPtr_ && "Data() requires mappedPtr_. Ensure DxBuffer maps upload resources.", "Assertion failed");
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
