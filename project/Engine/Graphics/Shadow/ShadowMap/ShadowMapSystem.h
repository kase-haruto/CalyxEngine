#pragma once

/* ========================================================================*/
/*		include space
/* ===================================================================== */
#include "Engine/Graphics/Buffer/DxStructuredBuffer.h"
#include "Engine/Graphics/Pipeline/PipelineType.h"
#include "Engine/Objects/Transform/Transform.h"
#include "GpuResource/ShadowMapResource.h"

#include <Engine/Foundation/Math/Matrix4x4.h>

namespace CalyxMath {
	struct Matrix4x4;
}

namespace CalyxAssets {
	class AnimationModel;
}

class PipelineService;
class BaseModel;

namespace CalyxGraphics {

	/*----------------------------------------------------------------------*
	 *	ShadowCBData
	 *	- シャドウマップ用定数バッファデータ
	 *---------------------------------------------------------------------*/
	struct ShadowCBData {
		CalyxMath::Matrix4x4 lightVP;
	};

	/*----------------------------------------------------------------------*
	 *	shadowMapSystem
	 *	- シャドウマップシステム
	 *---------------------------------------------------------------------*/
	class ShadowMapSystem {
	public:
		//===================================================================*/
		//				public methods
		//===================================================================*/
		ShadowMapSystem()  = default;
		~ShadowMapSystem() = default;
		/**
		 * \brief  初期化処理
		 * \param device
		 * \param size
		 */
		void Initialize(ID3D12Device* device, uint32_t size);
		/**
		 * \brief シャドウマップレンダリング
		 * \param cmdList
		 * \param psoService
		 * \param device
		 * \param staticVisible
		 * \param skinnedVisible
		 */
		void Render(
			ID3D12GraphicsCommandList*															 cmdList,
			PipelineService*																	 psoService,
			ID3D12Device*																		 device,
			const std::unordered_map<BaseModel*, std::vector<WorldTransform>>&					 staticVisible,
			const std::unordered_map<CalyxAssets::AnimationModel*, std::vector<WorldTransform>>& skinnedVisible);

		void BindForMainPass(ID3D12GraphicsCommandList* cmd);

		//--------- accessor -------------------------------------------
		/**
		 * \brief shadowMap を外から取得
		 * \return
		 */
		ShadowMapResource& GetShadowMap() { return shadowMap_; }
		/**
		 * \brief lightVP を外からセット
		 * \param lightVP
		 * \note LightLibraryで作った行列を渡す
		 */
		void SetLightVP(const CalyxMath::Matrix4x4& lightVP);

	private:
		//===================================================================*/
		//				private members
		//===================================================================*/
		ShadowMapResource					   shadowMap_; //< シャドウマップ用リソース
		DxConstantBuffer<ShadowCBData>		   shadowCB_;  //< シャドウマップ用定数バッファ
		
	};

} // namespace CalyxGraphics