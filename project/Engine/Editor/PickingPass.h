#pragma once

#include "Engine/Graphics/Descriptor/DescriptorAllocator.h"
#include <Engine/Renderer/Model/ModelRenderer.h>

#include <d3d12.h>
#include <wrl.h>

namespace CalyxEngine {
	/*-----------------------------------------------------------
	 * PickingPath
	 * - ピッキングパス定義クラス
	 * - マウスクリックなどでオブジェクト選択を行うためのレンダーパスを定義
	 *---------------------------------------------------------*/
	/**
	 * @brief PickingPassの機能を提供するクラスです。
	 */
	class PickingPass {
	public:
		//===================================================================*/
		//			public methods
		//===================================================================*/
		PickingPass()  = default;
		~PickingPass() = default;

		/**
		 * \brief 初期化処理
		 * \param width  幅
		 * \param height 高さ
		 */
		void Initialize(int32_t width, int32_t height);
		/**
		 * \brief 終了処理
		 */
		void Finalize();
		/**
		 * \brief リサイズ処理
		 * \param width  幅
		 * \param height 高さ
		 */
		void Resize(int32_t width, int32_t height);

		/**
		 * \brief ピッキング用テクスチャ描画
		 */
		void Render(ID3D12GraphicsCommandList* cmd,
					ModelRenderer*			   modelRenderer,
					PipelineService*		   psoService);
		void RenderDepthPreview(ID3D12GraphicsCommandList* cmd, const PipelineService* psoService);

		void TransitionColorTo(ID3D12GraphicsCommandList* cmd, D3D12_RESOURCE_STATES newState);
		void TransitionDepthTo(ID3D12GraphicsCommandList* cmd, D3D12_RESOURCE_STATES newState);
		void TransitionDepthPreviewTo(ID3D12GraphicsCommandList* cmd, D3D12_RESOURCE_STATES newState);

		// RT / Depth 取得（Readback用に後で使う）
		ID3D12Resource*				GetColor() const { return color_.Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetRtv() const { return rtv_.cpu; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDsv() const { return dsv_.cpu; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetSrv() const { return srv_.gpu; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetDepthSrv() const { return depthSrv_.gpu; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetDepthPreviewSrv() const { return depthPreviewSrv_.gpu; }

		uint32_t GetWidth() const { return width_; }
		uint32_t GetHeight() const { return height_; }

		/**
		 * \brief オブジェクトID取得
		 * \param x マップ座標X
		 * \param y マップ座標Y
		 * \return オブジェクトID
		 */
		uint32_t GetObjectID(int32_t x, int32_t y);
		/**
		 * \brief デフィバッファの値を取得
		 * \param x マップ座標X
		 * \param y マップ座標Y
		 * \return デプス値 (0.0 - 1.0)
		 */
		float GetDepth(int32_t x, int32_t y);

	private:
		void CreateResources(uint32_t w, uint32_t h);
		void DestroyResources();

		void CreateReadback(uint32_t w, uint32_t h);

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> color_;
		Microsoft::WRL::ComPtr<ID3D12Resource> depth_;
		Microsoft::WRL::ComPtr<ID3D12Resource> depthPreview_;
		Microsoft::WRL::ComPtr<ID3D12Resource> readback_;
		Microsoft::WRL::ComPtr<ID3D12Resource> readbackDepth_;

		DescriptorHandle rtv_{};
		DescriptorHandle dsv_{};
		DescriptorHandle srv_{};
		DescriptorHandle depthSrv_{};
		DescriptorHandle depthPreviewRtv_{};
		DescriptorHandle depthPreviewSrv_{};

		uint32_t width_			= 0;
		uint32_t height_		= 0;
		uint32_t rowPitch_		= 0;
		uint32_t rowPitchDepth_ = 0;
		D3D12_RESOURCE_STATES colorState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		D3D12_RESOURCE_STATES depthState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		D3D12_RESOURCE_STATES depthPreviewState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	};
} // namespace CalyxEngine
