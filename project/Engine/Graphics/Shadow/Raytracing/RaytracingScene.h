#pragma once

/* ========================================================================
/*	include space
/* ===================================================================== */
#include "Engine/Graphics/Buffer/DxStructuredBuffer.h"

#include <d3d12.h>
#include <vector>

namespace CalyxEngine {
	struct Matrix3x4;
}
namespace CalyxEngine {

	/*----------------------------------------------------------------
	 *	Raytracing Scene
	 *	- レイトレーシング用シーンデータ
	 *---------------------------------------------------------------*/
	class RaytracingScene {
	public:
		//===========================================================*/
		// public functions
		//===========================================================*/
		/**
		 * \brief インスタンス情報のクリア
		 */
		void Clear();
		/**
		 * \brief インスタンス情報の追加
		 * \param desc インスタンス情報
		 */
		void AddInstance(
			const CalyxEngine::Matrix3x4& transform,
			D3D12_GPU_VIRTUAL_ADDRESS   blasAddress,
			uint32_t                   instanceID,
			uint8_t                    mask = 0xFF,
			D3D12_RAYTRACING_INSTANCE_FLAGS flags =
				D3D12_RAYTRACING_INSTANCE_FLAG_NONE
		);
		/**
		 * \brief インスタンスバッファの確保
		 * \param device
		 */
		void EnsureBuffer(ID3D12Device* device);
		/**
		 * \brief インスタンスデータのアップロード
		 */
		void Upload();
		/**
		 * \brief インスタンス数の取得
		 * \return インスタンス数
		 */
		uint32_t GetInstanceCount() const;
		/**
		 * \brief インスタンスデスクリプタのGPU仮想アドレス取得
		 * \return GPU仮想アドレス
		 */
		D3D12_GPU_VIRTUAL_ADDRESS GetInstanceDescGPUVA() const;

	private:
		//===========================================================*/
		// private members
		//===========================================================*/
		std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances_;
		DxStructuredBuffer<D3D12_RAYTRACING_INSTANCE_DESC> instanceBuffer_;
	};

} // namespace CalyxEngine
