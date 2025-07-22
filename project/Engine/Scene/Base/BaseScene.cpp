#include "BaseScene.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Assets/Model/Model.h>
#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Assets/Model/ModelData.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Graphics/Pipeline/Presets/PipelinePresets.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Application/Effects/FxSystem.h>

BaseScene::BaseScene(){
	sceneContext_ = std::make_unique<SceneContext>();

	skyBox_ = std::make_unique<SkyBox>("sky.dds", "skyBox");
	skyBox_->Initialize();

	spriteRenderer_ = std::make_unique<SpriteRenderer>();
	modelRenderer_ = std::make_unique<ModelRenderer>();
}

void BaseScene::Draw(ID3D12GraphicsCommandList* cmdList,
					 PipelineService* psoService,
					 [[maybe_unused]]RenderTargetType renderTargetType){
	//===================================================================*/
	//						背景オブジェクト描画
	//===================================================================*/
	cmdList->SetGraphicsRootSignature(
		GraphicsGroup::GetInstance()->GetRootSignature(PipelineType::Skybox).Get());
	skyBox_->Draw(cmdList);

	//===================================================================*/
	//						シーンオブジェクトの描画
	//===================================================================*/
	modelRenderer_->Clear();

	for (auto* entry : sceneContext_->GetObjectLibrary()->GetAllObjectsRaw()){
		auto* gameObj = dynamic_cast< BaseGameObject* >(entry);
		if (!gameObj) continue;

		switch (gameObj->GetModelType()){
			case ObjectModelType::ModelType_Static:
				if (auto* model = gameObj->GetStaticModel()){
					modelRenderer_->RegisterStatic(model, gameObj->GetWorldTransform());
				}
				break;
			case ObjectModelType::ModelType_Animation:
				if (auto* model = gameObj->GetAnimationModel()){
					modelRenderer_->RegisterSkinned(model, gameObj->GetWorldTransform());
				}
				break;
			default:
				break;
		}
	}

	//======================== モデル描画 ========================//
	const Camera3d* camera = CameraManager::GetInstance()->GetCamera3d();
	modelRenderer_->DrawAll(cmdList,
							GraphicsGroup::GetInstance()->GetDevice().Get(),
							camera,
							psoService,
							sceneContext_->GetLightLibrary());


	//===================================================================*/
	//						sprite
	//===================================================================*/
	spriteRenderer_->Draw(cmdList, psoService, renderTargetType);

	//===================================================================*/
	//                    particle描画
	//===================================================================*/
	sceneContext_->GetFxSystem()->Render(psoService, cmdList);
}
