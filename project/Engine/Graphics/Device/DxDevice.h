#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

namespace CalyxGraphics {

	/*----------------------------------------------------------------
	 *	Dx Device
	 *	- DirectX12デバイス管理
	 *---------------------------------------------------------------*/
	class DxDevice {
		template <class T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		/*-----------------------------------------------------------
		 *	Capabilities
		 *	- デバイスの機能情報
		 *----------------------------------------------------------*/
		struct Capabilities {
			D3D12_RAYTRACING_TIER raytracingTier = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
			bool shaderModel6_5 = false; // RayQuery 前提
		};
		
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
		void Initialize();

		//- accessors ------------------------------------------------//
		// getter
		const ComPtr<ID3D12Device>&	 GetDevice() const { return device_; }
		const ComPtr<IDXGIFactory7>& GetDXGIFactory() const { return dxgiFactory_; }
		const Capabilities& GetCaps() const { return caps_; } 
		bool IsInlineRaytracingSupported() const;
		
	private:
		//===========================================================*/
		// private methods
		//===========================================================*/
		/**
		 * \brief デバッグレイヤーのセットアップ
		 */
		void SetupDebugLayer();
		/**
		 * \brief DXGIデバイスの生成
		 */
		void CreateDXGIDevice();
		/**
		 * \brief レイトレーシング機能確認
		 * \note DXR対応確認など
		 */
		void QueryCapabilities();
	private:
		//==========================================================*/
		// private members
		//==========================================================*/
		ComPtr<ID3D12Device>  device_		   = nullptr; //< D3D12デバイス
		ComPtr<IDXGIAdapter4> adapter_		   = nullptr; //< アダプター
		ComPtr<IDXGIFactory7> dxgiFactory_	   = nullptr; //< DXGIファクトリー
		ComPtr<ID3D12Debug1>  debugController_ = nullptr; //< デバッグコントローラー
		ComPtr<ID3D12Device5> device5_		   = nullptr; //< D3D12デバイス5 (DXR用)

		Capabilities caps_{}; // デバイスの機能情報
	};

} // namespace CalyxGraphics