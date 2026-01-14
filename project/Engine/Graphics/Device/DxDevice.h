#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

namespace CalyxGraphics {

	/*-----------------------------------------------------------------------------------------
	 * DxDevice
	 * - DirectX12デバイス管理クラス
	 * - デバイスの生成、アダプターの選択、機能情報の管理を担当
	 *---------------------------------------------------------------------------------------*/
	class DxDevice {
		template <class T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;

		/*-----------------------------------------------------------------------------------------
		 * Capabilities
		 * - デバイスの機能情報構造体
		 *---------------------------------------------------------------------------------------*/
		struct Capabilities {
			D3D12_RAYTRACING_TIER raytracingTier = D3D12_RAYTRACING_TIER_NOT_SUPPORTED; //< レイトレーシングティア
			bool shaderModel6_5 = false; //< シェーダーモデル6.5のサポートフラグ
		};
		
	public:
		//===================================================================*/
		//		public functions
		//===================================================================*/
		/**
		 * \brief コンストラクタ
		 */
		DxDevice() = default;

		/**
		 * \brief デストラクタ
		 */
		~DxDevice();

		/**
		 * \brief 初期化
		 */
		void Initialize();

		//===================================================================*/
		//		accessor
		//===================================================================*/
		/**
		 * \brief D3D12デバイスを取得
		 * \return デバイス
		 */
		const ComPtr<ID3D12Device>&	 GetDevice() const { return device_; }

		/**
		 * \brief D3D12デバイス5を取得
		 * \return デバイス5
		 */
		ID3D12Device5* GetDevice5(){return device5_.Get();}

		/**
		 * \brief DXGIファクトリーを取得
		 * \return DXGIファクトリー
		 */
		const ComPtr<IDXGIFactory7>& GetDXGIFactory() const { return dxgiFactory_; }

		/**
		 * \brief デバイス機能情報を取得
		 * \return 機能情報
		 */
		const Capabilities& GetCaps() const { return caps_; } 

		/**
		 * \brief インラインレイトレーシングがサポートされているか
		 * \return サポートされているか
		 */
		bool IsInlineRaytracingSupported() const;
		
	private:
		//===================================================================*/
		//		private methods
		//===================================================================*/
		/**
		 * \brief デバッグレイヤーのセットアップ
		 */
		void SetupDebugLayer();

		/**
		 * \brief DXGIデバイスの生成
		 */
		void CreateDXGIDevice();

		/**
		 * \brief 機能情報の問い合わせ
		 */
		void QueryCapabilities();

	private:
		//===================================================================*/
		//		private member variables
		//===================================================================*/
		ComPtr<ID3D12Device>  device_		   = nullptr; //< D3D12デバイス
		ComPtr<IDXGIAdapter4> adapter_		   = nullptr; //< アダプター
		ComPtr<IDXGIFactory7> dxgiFactory_	   = nullptr; //< DXGIファクトリー
		ComPtr<ID3D12Debug1>  debugController_ = nullptr; //< デバッグコントローラー
		ComPtr<ID3D12Device5> device5_		   = nullptr; //< D3D12デバイス5 (DXR用)

		Capabilities caps_{}; //< デバイスの機能情報
	};

} // namespace CalyxGraphics