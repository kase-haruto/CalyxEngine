#pragma once
#include "Engine/Assets/Model/ModelData.h"

#include <d3d12.h>
#include <vector>
#include <wrl.h>

namespace CalyxEngine {

	/*----------------------------------------------------------------
	 *	Raytracing Mesh
	 *	- レイトレーシング用メッシュデータ
	 *---------------------------------------------------------------*/
	/**
	 * @brief RaytracingMeshの機能を提供するクラスです。
	 */
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
		void                      BuildBLAS(ID3D12Device5* device,ID3D12GraphicsCommandList4* cmd,const std::vector<VertexPosUvN>& vertices,const std::vector<uint32_t>& indices);
		void                      BuildBLAS(ID3D12Device5* device,ID3D12GraphicsCommandList4* cmd,D3D12_GPU_VIRTUAL_ADDRESS vertexAddress,UINT vertexCount,UINT vertexStride,D3D12_GPU_VIRTUAL_ADDRESS indexAddress,UINT indexCount);
		/**
		 * \brief BLASの取得
		 * \return  BLASのGPU仮想アドレス
		 */
		D3D12_GPU_VIRTUAL_ADDRESS GetBLAS() const;

	private:
		void RetireResource(Microsoft::WRL::ComPtr<ID3D12Resource>& resource);

		//===========================================================*/
		// private members
		//===========================================================*/
		Microsoft::WRL::ComPtr<ID3D12Resource> blas_;		//< BLAS
		Microsoft::WRL::ComPtr<ID3D12Resource> scratch_;	//< スクラッチバッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexUpload_;
		Microsoft::WRL::ComPtr<ID3D12Resource> indexUpload_;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> retiredResources_;
	};

} // namespace CalyxEngine
