#pragma once
#include <cassert>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>

namespace CalyxGraphics {

	/*----------------------------------------------------------------
	 *	Dx Device
	 *	- DirectX12デバイス管理
	 *---------------------------------------------------------------*/
	class DxDevice {
		template <class T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

	public:
		///===========================================================*/
		/// public functions
		///===========================================================*/
		/** \brief コンストラクタ・デストラクタ */
		DxDevice() = default;
		~DxDevice();

		/**
		 * \brief 初期化
		 */
		void						 Initialize();

		//- accessors --------------------------------//
		// getter
		const ComPtr<ID3D12Device>&	 GetDevice() const { return device_; }
		const ComPtr<IDXGIFactory7>& GetDXGIFactory() const { return dxgiFactory_; }

	private:
		/**
		 * \brief デバッグレイヤーのセットアップ
		 */
		void SetupDebugLayer();
		/**
		 * \brief DXGIデバイスの生成
		 */
		void CreateDXGIDevice();

	private:
		///////////////////////////////////////////////////
		//              リソース
		///////////////////////////////////////////////////
		ComPtr<ID3D12Device>  device_		   = nullptr;	//< D3D12デバイス
		ComPtr<IDXGIAdapter4> adapter_		   = nullptr;	//< アダプター
		ComPtr<IDXGIFactory7> dxgiFactory_	   = nullptr;	//< DXGIファクトリー
		ComPtr<ID3D12Debug1>  debugController_ = nullptr;	//< デバッグコントローラー
	};

} // namespace CalyxGraphics