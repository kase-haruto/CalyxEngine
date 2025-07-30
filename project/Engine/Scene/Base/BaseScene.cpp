#include "BaseScene.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Assets/Model/Model.h>
#include <Engine/Assets/Model/ModelData.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Pipeline/Presets/PipelinePresets.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Scene/Utility/SceneUtility.h>

/////////////////////////////////////////////////////////////////////////////////////////
//コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
BaseScene::BaseScene() {
	spriteRenderer_ = std::make_unique<SpriteRenderer>();
	modelRenderer_ = std::make_unique<ModelRenderer>();
}

void BaseScene::Initialize() {
	playSession_.Initialize(sceneContext_);
	skyBox_ = SceneAPI::Instantiate<SkyBox>("sky.dds", "skyBox");
	skyBox_->Initialize();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新前処理
/////////////////////////////////////////////////////////////////////////////////////////
void BaseScene::PostUpdate([[maybe_unused]] ID3D12GraphicsCommandList* cmdList,
						   [[maybe_unused]] PipelineService* psoService) {

	SceneContext* ctx = ActiveCtx();
	ctx->MakeCurrent();
	ctx->PostUpdate(psoService, cmdList);

}

/////////////////////////////////////////////////////////////////////////////////////////
//		描画
/////////////////////////////////////////////////////////////////////////////////////////
void BaseScene::Draw(ID3D12GraphicsCommandList* cmd,
					 PipelineService* pso,
					 RenderTargetType) {
	/* 1) Skybox */
	cmd->SetGraphicsRootSignature(
		GraphicsGroup::GetInstance()->GetRootSignature(PipelineType::Skybox).Get());
	skyBox_->Draw(cmd);

	/* 2) Scene objects */
	SceneContext* ctx = ActiveCtx();
	modelRenderer_->BeginFrame();

	for (auto* e : ctx->GetObjectLibrary()->GetAllObjectsRaw()) {
		if (auto* go = dynamic_cast<BaseGameObject*>(e)) {
			switch (go->GetModelType()) {
				case ObjectModelType::ModelType_Static:
					if (auto* m = go->GetStaticModel())
						modelRenderer_->RegisterStatic(m, go->GetWorldTransform());
					break;
				case ObjectModelType::ModelType_Animation:
					if (auto* m = go->GetAnimationModel())
						modelRenderer_->RegisterSkinned(m, go->GetWorldTransform());
					break;
				default: break;
			}
		}
	}

	const Camera3d* cam = static_cast<Camera3d*>(CameraManager::GetMain3d());
	modelRenderer_->PreCullAndBatch(cam);
	modelRenderer_->DrawAll(cmd,
							GraphicsGroup::GetInstance()->GetDevice().Get(),
							cam,
							pso,
							ctx->GetLightLibrary());

	/* 3) Particles */
	ctx->GetFxSystem()->Render(pso, cmd);
}

void BaseScene::DrawSpritesOnly(ID3D12GraphicsCommandList* cmdList,
								PipelineService* psoService) {
	spriteRenderer_->Draw(cmdList, psoService, RenderTargetType::BackBuffer);
}
