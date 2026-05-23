#pragma once
#include "Engine/Graphics/Device/DxDevice.h"

#include <cstdint>

namespace CalyxEngine {
	class SceneDepthResource {
	public:
		void Initialize(ID3D12Device* device, uint32_t w, uint32_t h) ;

		D3D12_CPU_DESCRIPTOR_HANDLE GetDsv() const { return dsv_; }
		void Transition(
			ID3D12GraphicsCommandList* cmd,
			D3D12_RESOURCE_STATES newState);

		void BindForWrite(ID3D12GraphicsCommandList* cmd);
	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
		D3D12_CPU_DESCRIPTOR_HANDLE dsv_{};

		D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	};

}


