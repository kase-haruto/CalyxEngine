#include "ShadowMapSystem.h"

#include "Engine/Assets/Animation/AnimationModel.h"
#include "Engine/graphics/Pipeline/BlendMode/BlendMode.h"
#include "Engine/Graphics/Pipeline/Service/PipelineService.h"

namespace CalyxGraphics {

	/////////////////////////////////////////////////////////////////////////////////////
	//		シャドウマップシステム初期化
	/////////////////////////////////////////////////////////////////////////////////////
	void ShadowMapSystem::Initialize(ID3D12Device* device,uint32_t size) {
		shadowMap_.Initialize(device,size,size);
		shadowCB_.Initialize(device);
		worldCB_.Initialize(device);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	//		デプスマップ描画
	//////////////////////////////////////////////////////////////////////////////////////
	void ShadowMapSystem::Render(ID3D12GraphicsCommandList*                                        cmdList,
								 PipelineService*                                                  psoService,
								 ID3D12Device*                                                    /*device*/ ,
								 const std::unordered_map<BaseModel*,std::vector<WorldTransform>>& staticVisible,
								 const std::unordered_map<CalyxAssets::AnimationModel*,
														  std::vector<WorldTransform>>& skinnedVisible) {

		shadowMap_.BeginShadowPass(cmdList);

		// ---- Static ----
		{
			auto ps = psoService->GetPipelineSet(PipelineTag::Object::ShadowStatic,BlendMode::NONE);
			psoService->SetCommand(ps,cmdList);

			shadowCB_.SetCommand(cmdList,0);

			for(auto& [model, tfs] : staticVisible) {
				if(!model || !model->GetModelData().has_value()) continue;

				model->BindVertexIndexBuffers(cmdList);
				const UINT indexCount = (UINT)model->GetModelData()->meshData.indices.size();

				for(auto& tf : tfs) {
					worldCB_.TransferData(tf.matrix.world);
					worldCB_.SetCommand(cmdList,1);

					cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					cmdList->DrawIndexedInstanced(indexCount,1,0,0,0);
				}
			}
		}

		// ---- Skinned ----
		{
			auto ps = psoService->GetPipelineSet(PipelineTag::Object::ShadowSkinned,BlendMode::NONE);
			psoService->SetCommand(ps,cmdList);

			shadowCB_.SetCommand(cmdList,0);

			for(auto& [model, tfs] : skinnedVisible) {
				if(!model || !model->GetModelData().has_value()) continue;

				cmdList->SetGraphicsRootDescriptorTable(2,model->GetJointMatrixSrv());

				model->BindVertexIndexBuffers(cmdList);
				const UINT indexCount = (UINT)model->GetModelData()->meshData.indices.size();

				for(auto& tf : tfs) {
					worldCB_.TransferData(tf.matrix.world);
					worldCB_.SetCommand(cmdList,1);

					cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					cmdList->DrawIndexedInstanced(indexCount,1,0,0,0);
				}
			}
		}

		shadowMap_.EndShadowPass(cmdList);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	//		ライトビュー・プロジェクション行列セット
	//////////////////////////////////////////////////////////////////////////////////////
	void ShadowMapSystem::SetLightVP(const CalyxMath::Matrix4x4& lightVP) {
		shadowCB_.TransferData({ lightVP });
	}
}