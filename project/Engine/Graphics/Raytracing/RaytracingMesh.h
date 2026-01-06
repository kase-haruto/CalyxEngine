#pragma once
#include "Engine/Assets/Model/ModelData.h"

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
		 * \brief BLASの構築
		 * \param device       デバイス
		 * \param cmd          コマンドリスト
		 * \param model        モデルデータ
		 */
		void                      BuildBLAS(ID3D12Device5* device,ID3D12GraphicsCommandList4* cmd,const ModelData& model);
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