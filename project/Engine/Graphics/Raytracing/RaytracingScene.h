#pragma once

/* ========================================================================
/*	include space
/* ===================================================================== */
#include <vector>
#include <d3d12.h>

namespace CalyxGraphics {

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
		void AddInstance(const D3D12_RAYTRACING_INSTANCE_DESC& desc);
		/**
		 * \brief インスタンス情報の取得
		 * \return インスタンス情報の配列
		 */
		const std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& GetInstances() const;
		
	private:
		//===========================================================*/
		// private members
		//===========================================================*/
		std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances_;	//< インスタンス情報の配列
	};

} // namespace CalyxGraphics
