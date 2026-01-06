#pragma once
#include "Engine/Graphics/Device/DxDevice.h"

#include <cstdint>

namespace CalyxGraphics {

	/*----------------------------------------------------------------------
	 *	shadowMapResource
	 *	- シャドウマップ用のGPUリソース
	 *--------------------------------------------------------------------*/
	class ShadowMapResource {
	public:
		//===================================================================*/
		//				public methods
		//===================================================================*/
		/** コンストラクタ / デストラクタ */
		ShadowMapResource()  = default;
		~ShadowMapResource() = default;
		/**
		 * \brief 初期化処理
		 * \param device
		 * \param width
		 * \param height
		 */
		void Initialize(
			ID3D12Device* device,
			uint32_t      width,
			uint32_t      height
			);
		/**
		 * \brief syadowPath 開始
		 * \param cmdList
		 */
		void BeginShadowPass(ID3D12GraphicsCommandList* cmdList);
		/**
		 * \brief shadowPath 終了
		 * \param cmdList
		 */
		void EndShadowPass(ID3D12GraphicsCommandList* cmdList);

		//--------- accessor -------------------------------------------
		D3D12_GPU_DESCRIPTOR_HANDLE GetSrv() const { return srvGpu_; }
		const D3D12_VIEWPORT& GetViewport() const { return viewport_; }
		const D3D12_RECT&     GetScissor() const { return scissor_; }

	private:
		//===================================================================*/
		//				private members
		//===================================================================*/
		// GPU Resource
		Microsoft::WRL::ComPtr<ID3D12Resource> resource_;

		// state
		D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;

		// Descriptor Handles
		D3D12_CPU_DESCRIPTOR_HANDLE dsv_{};
		D3D12_CPU_DESCRIPTOR_HANDLE srvCpu_{};
		D3D12_GPU_DESCRIPTOR_HANDLE srvGpu_{};

		D3D12_VIEWPORT viewport_{};
		D3D12_RECT     scissor_{};
	};

}