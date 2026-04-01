#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/Platform/WinApp.h>
#include <Engine/Graphics/Context/DxCommand.h>
#include <Engine/Graphics/Context/DxFence.h>
#include <Engine/Graphics/Device/DxDevice.h>
#include <Engine/Graphics/RenderTarget/Collection/RenderTargetCollection.h>
#include <Engine/Graphics/ResourceStateTracker/ResourceStateTracker.h>
#include <Engine/Graphics/SwapChain/DxSwapChain.h>

// c++
#include <memory>

using Microsoft::WRL::ComPtr;

namespace CalyxGraphics {

	/*-----------------------------------------------------------------------------------------
	 * DxCore
	 * - DirectX12のコア機能をまとめたクラス
	 * - デバイス、コマンドリスト、スワップチェーンなどのライフサイクル管理を担当
	 *---------------------------------------------------------------------------------------*/
	class DxCore {
	public:
		//===================================================================*/
		//		public methods
		//===================================================================*/
		DxCore() = default;
		~DxCore();

		/**
		 * \brief 初期化
		 * \param winApp ウィンドウアプリケーション
		 * \param width 画面幅
		 * \param height 画面高さ
		 */
		void Initialize(WinApp* winApp,uint32_t width,uint32_t height);

		/**
		 * \brief レンダラ初期化
		 * \param width 画面幅
		 * \param height 画面高さ
		 */
		void RendererInitialize(uint32_t width,uint32_t height);

		/**
		 * \brief 描画前処理
		 */
		void PreDraw();

		/**
		 * \brief オフロード用描画前処理
		 */
		void PreDrawOffscreen();

		/**
		 * \brief 描画後処理
		 */
		void PostDraw();

		/**
		 * \brief リサイズ
		 * \param width 幅
		 * \param height 高さ
		 */
		void Resize(uint32_t width, uint32_t height);

		/**
		 * \brief エンジンUIの描画
		 */
		void RenderEngineUI();

	private:
		//===================================================================*/
		//		private methods
		//===================================================================*/

		/**
		 * \brief リソース解放
		 */
		void ReleaseResources();

	public:
		//===================================================================*/
		//		accessor
		//===================================================================*/
		/**
		 * \brief デバイスを取得
		 * \return デバイス
		 */
		const ComPtr<ID3D12Device>&              GetDevice() const { return dxDevice_->GetDevice(); }
		/**
		 * \brief ID3D12Device5インターフェースを取得
		 * \return デバイス5
		 */
		ID3D12Device5*                           GetDevice5()const { return dxDevice_->GetDevice5(); }
		/**
		 * \brief グラフィックスコマンドリストを取得
		 * \return コマンドリスト
		 */
		const ComPtr<ID3D12GraphicsCommandList>& GetCommandList() const { return dxCommand_->GetCommandList(); }
		/**
		 * \brief スワップチェーンを取得
		 * \return スワップチェーン
		 */
		const DxSwapChain&                       GetSwapChain() const { return *dxSwapChain_; }
		/**
		 * \brief レンダリングターゲットコレクションを取得
		 * \return レンダリングターゲットコレクション
		 */
		const RenderTargetCollection&            GetRenderTargetCollection() const { return *renderTargetCollection_; }
		/**
		 * \brief 画面フォーマットを取得
		 * \return フォーマット
		 */
		DXGI_FORMAT GetFormat() const { return format_; }

		uint32_t GetClientWidth() const { return clientWidth_; }
		uint32_t GetClientHeight() const { return clientHeight_; }

	private:
		//===================================================================*/
		//		private member variables
		//===================================================================*/
		WinApp*  winApp_       = nullptr; //< ウィンドウアプリケーション
		uint32_t clientWidth_  = 0; //< クライアント領域の幅
		uint32_t clientHeight_ = 0; //< クライアント領域の高さ

		std::unique_ptr<DxDevice>    dxDevice_; //< DirectXデバイス
		std::unique_ptr<DxCommand>   dxCommand_; //< コマンド管理
		std::unique_ptr<DxSwapChain> dxSwapChain_; //< スワップチェーン

		DXGI_FORMAT                             format_            = DXGI_FORMAT_R8G8B8A8_UNORM; //< バックバッファフォーマット
		std::unique_ptr<RenderTargetCollection> renderTargetCollection_; //< レンダリングターゲット管理
		std::unique_ptr<DxFence>                dxFence_; //< フェンス管理
	};

} // namespace CalyxGraphics