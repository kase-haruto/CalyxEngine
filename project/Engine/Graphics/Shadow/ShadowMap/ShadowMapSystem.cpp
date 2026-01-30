#include "ShadowMapSystem.h"

#include "Engine/Assets/Animation/AnimationModel.h"
#include "Engine/Graphics/Context/GraphicsGroup.h"
#include "Engine/Graphics/Pipeline/Service/PipelineService.h"
#include "Engine/graphics/Pipeline/BlendMode/BlendMode.h"

namespace CalyxGraphics {

	/////////////////////////////////////////////////////////////////////////////////////
	//		シャドウマップシステム初期化
	/////////////////////////////////////////////////////////////////////////////////////
	void CalyxGraphics::ShadowMapSystem::Initialize(ID3D12Device* device,uint32_t size) {
		shadowMap_.Initialize(device,size,size);
		shadowCB_.Initialize(device);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	//		デプスマップ描画
	//////////////////////////////////////////////////////////////////////////////////////
	void ShadowMapSystem::Render(
		ID3D12GraphicsCommandList* cmdList,
		PipelineService*           psoService,
		ID3D12Device* /*device*/,
		const std::unordered_map<BaseModel*,std::vector<WorldTransform>>&                   staticVisible,
		const std::unordered_map<CalyxAssets::AnimationModel*,std::vector<WorldTransform>>& skinnedVisible) {
		psoService->ResetState();
		shadowMap_.BeginShadowPass(cmdList);

		// ---- Static ----
		{
			auto ps = psoService->GetPipelineSet(PipelineTag::Object::ShadowStatic,BlendMode::NONE);
			psoService->SetCommand(ps,cmdList);

			// b0 : ShadowCB
			shadowCB_.SetCommand(cmdList,0);

			for(const auto& [model, tfs] : staticVisible) {
				if(!model || !model->GetModelData()) { continue; }
				if(tfs.empty()) { continue; }

				cmdList->SetGraphicsRootDescriptorTable(1,model->GetInstanceSrv());

				model->BindVertexIndexBuffers(cmdList);
				const UINT indexCount = (UINT)model->GetModelData()->meshResource.Indices().size();

				cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				cmdList->DrawIndexedInstanced(indexCount,(UINT)tfs.size(),0,0,0);
			}
		}

		// ---- Skinned ----
		{
			auto ps = psoService->GetPipelineSet(PipelineTag::Object::ShadowSkinned,BlendMode::NONE);
			psoService->SetCommand(ps,cmdList);

			shadowCB_.SetCommand(cmdList,0);

			for(const auto& [model, tfs] : skinnedVisible) {
				if(!model || !model->GetModelData()) continue;

				cmdList->SetGraphicsRootDescriptorTable(2,model->GetJointMatrixSrv());

				// skin pallet
				model->SetCommandPalletSrv(2,cmdList);

				model->BindVertexIndexBuffers(cmdList);
				const UINT indexCount = (UINT)model->GetModelData()->meshResource.Indices().size();

				for(const auto& tf : tfs) {
					(void)tf;
					tf.SetCommand(cmdList,1);
					cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					cmdList->DrawIndexedInstanced(indexCount,1,0,0,0);
				}
			}
		}

		shadowMap_.EndShadowPass(cmdList);
	}

	void ShadowMapSystem::BindForMainPass(ID3D12GraphicsCommandList* cmd) {
		// ShadowMap SRV
		cmd->SetGraphicsRootDescriptorTable(
			9,
			shadowMap_.GetSrv());

		// LightVP
		shadowCB_.SetCommand(cmd,8);
	}

	void ShadowMapSystem::UpdateShadowBounds(const Camera3d& camera,float shadowFar,float expandMargin) {
		shadowBounds_.UpdateFromCamera(camera,shadowFar,expandMargin);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	//		ライトビュー・プロジェクション行列セット
	//////////////////////////////////////////////////////////////////////////////////////
	void ShadowMapSystem::SetLightVP(const CalyxMath::Matrix4x4& lightVP) {
		cbData_.lightVP = lightVP;
		shadowCB_.TransferData(cbData_);
	}
}