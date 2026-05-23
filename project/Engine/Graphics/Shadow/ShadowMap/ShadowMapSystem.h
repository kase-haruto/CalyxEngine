#pragma once

/* ========================================================================*/
/*		include space
/* ===================================================================== */
#include "Engine/Graphics/Buffer/DxStructuredBuffer.h"
#include "Engine/Graphics/Pipeline/PipelineType.h"
#include "Engine/Objects/Transform/Transform.h"
#include "GpuResource/ShadowMapResource.h"
#include <Engine/Graphics/Shadow/ShadowMap/ShadowBounds.h>
#include <Engine/Foundation/Math/Matrix4x4.h>

namespace CalyxEngine {
	struct Matrix4x4;
}

namespace CalyxEngine {
	class AnimationModel;
}

class PipelineService;
class BaseModel;

namespace CalyxEngine {

	/*----------------------------------------------------------------------*
	 *	ShadowCBData
	 *	- シャドウマップ用定数バッファデータ
	 *---------------------------------------------------------------------*/
	struct ShadowCBData {
		CalyxEngine::Matrix4x4 lightVP;
		float shadowBias = 0.001f;
		float padding[3];
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
		void Initialize(ID3D12Device* device,uint32_t size);
		/**
		 * \brief シャドウマップレンダリング
		 * \param cmdList
		 * \param psoService
		 * \param device
		 * \param staticVisible
		 * \param skinnedVisible
		 */
		void Render(
			ID3D12GraphicsCommandList*                                                          cmdList,
			PipelineService*                                                                    psoService,
			ID3D12Device*                                                                       device,
			const std::unordered_map<BaseModel*,std::vector<WorldTransform>>&                   staticVisible,
			const std::unordered_map<CalyxEngine::AnimationModel*,std::vector<WorldTransform>>& skinnedVisible);

		void BindForMainPass(ID3D12GraphicsCommandList* cmd);

		void UpdateShadowBounds(const class Camera3d& camera, float shadowFar, float expandMargin = 5.0f);

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
		void SetLightVP(const CalyxEngine::Matrix4x4& lightVP);

		const ShadowBounds& GetShadowBounds()const { return shadowBounds_; }

		void SetShadowBias(float bias) {
			cbData_.shadowBias = bias;
			shadowCB_.TransferData(cbData_);
		}

	private:
		//===================================================================*/
		//				private members
		//===================================================================*/
		ShadowMapResource              shadowMap_;    //< シャドウマップ用リソース
		DxConstantBuffer<ShadowCBData> shadowCB_;     //< シャドウマップ用定数バッファ
		ShadowCBData                   cbData_;
		ShadowBounds                   shadowBounds_; //< シャドウ範囲管理
	};

} // namespace CalyxEngine