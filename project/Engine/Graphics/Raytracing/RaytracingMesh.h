#pragma once
#include <d3d12.h>
#include <wrl.h>

namespace CalyxGraphics {

	/*----------------------------------------------------------------
	 *	Raytracing Mesh
	 *	- レイトレーシング用メッシュデータ
	 *---------------------------------------------------------------*/
	class RaytracingMesh {
	public:
		//===========================================================*/
		// public functions
		//===========================================================*/

		/**
		 * \brief 構築
		 * \param device
		 * \param cmd
		 * \param vb
		 * \param vbStride
		 * \param vbCount
		 * \param ib
		 * \param ibFormat
		 * \param ibCount
		 */
		void Build(
			ID3D12Device5*				device,
			ID3D12GraphicsCommandList4* cmd,
			D3D12_GPU_VIRTUAL_ADDRESS	vb,
			UINT						vbStride,
			UINT						vbCount,
			D3D12_GPU_VIRTUAL_ADDRESS	ib,
			DXGI_FORMAT					ibFormat,
			UINT						ibCount);

		/**
		 * \brief BLASの取得
		 * \return  BLASのGPU仮想アドレス
		 */
		D3D12_GPU_VIRTUAL_ADDRESS GetBLAS() const;

	private:
		//===========================================================*/
		// private members
		//===========================================================*/
		Microsoft::WRL::ComPtr<ID3D12Resource> blas_;		//< BLAS
		Microsoft::WRL::ComPtr<ID3D12Resource> scratch_;	//< スクラッチバッファ
	};

} // namespace CalyxGraphics