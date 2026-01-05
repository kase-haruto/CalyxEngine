#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <d3d12.h>
#include <wrl.h>

namespace CalyxGraphics {

	// 前方宣言
	class RaytracingScene;

	/*----------------------------------------------------------------
	 *	Raytracing System
	 *	- レイトレーシングシステム
	 *	- DXRを知る唯一のクラス
	 *---------------------------------------------------------------*/
	class RaytracingSystem {
	public:
		//===========================================================*/
		// public functions
		//===========================================================*/
		/**
		 * \brief 初期化
		 * \param device
		 */
		void Initialize(ID3D12Device5* device);
		/**
		 * \brief TLASの構築
		 * \param cmd
		 * \param scene
		 */
		void BuildTLAS(
			ID3D12GraphicsCommandList4* cmd,
			const RaytracingScene& scene
		);
		/**
		 * \brief TLASの取得
		 * \return TLASのGPU仮想アドレス
		 */
		D3D12_GPU_DESCRIPTOR_HANDLE GetTLASSrv() const;
		
	private:
		//===========================================================*/
		// private members
		//===========================================================*/
		Microsoft::WRL::ComPtr<ID3D12Resource> tlas_;		//< TLAS
		Microsoft::WRL::ComPtr<ID3D12Resource> scratch_;	//< スクラッチバッファ
		D3D12_GPU_DESCRIPTOR_HANDLE tlasSrv_{};				//< TLASのSRV
	};

	
}

