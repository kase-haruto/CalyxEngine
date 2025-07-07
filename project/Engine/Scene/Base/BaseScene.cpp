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
}

void BaseScene::Draw(ID3D12GraphicsCommandList* cmdList,
					 PipelineService* psoService,
					 RenderTargetType renderTargetType){
	//===================================================================*/
	//						背景オブジェクト描画
	//===================================================================*/
	cmdList->SetGraphicsRootSignature(
		GraphicsGroup::GetInstance()->GetRootSignature(PipelineType::Skybox).Get());
	skyBox_->Draw(cmdList);

	//===================================================================*/
	//						シーンオブジェクトの描画
	//===================================================================*/
	std::vector<std::pair<Model*, WorldTransform*>> staticModels;
	std::vector<std::pair<AnimationModel*, WorldTransform*>> skinnedModels;

	for (auto* entry : sceneContext_->GetObjectLibrary()->GetAllObjects()){
		auto* gameObj = dynamic_cast< BaseGameObject* >(entry);
		if (!gameObj) continue;

		switch (gameObj->GetModelType()){
			case ObjectModelType::ModelType_Static:
				if (auto* model = gameObj->GetStaticModel()){
					staticModels.emplace_back(model, &gameObj->GetWorldTransform());
				}
				break;
			case ObjectModelType::ModelType_Animation:
				if (auto* model = gameObj->GetAnimationModel()){
					skinnedModels.emplace_back(model, &gameObj->GetWorldTransform());
				}
				break;
			default:
				break;
		}
	}

	const Camera3d* camera = CameraManager::GetInstance()->GetCamera3d();

	//===================================================================*/
	// 静的モデル描画（視錐台カリング追加）
	//===================================================================*/
	for (const auto& [model, transform] : staticModels){
		if (model->GetModelData() == std::nullopt) continue;

		// ワールド行列取得
		const Matrix4x4& worldMat = transform->matrix.world;

		// ローカルAABBをワールド変換
		AABB worldAABB = model->GetModelData()->localAABB.Transform(worldMat);

		// 視錐台カリング判定
		if (!camera->IsVisible(worldAABB)){
			continue; // カリングされたので描画スキップ
		}

		BlendMode mode = model->GetBlendMode();
		auto desc = PipelinePresets::MakeObject3D(mode);

		psoService->SetCommand(desc, cmdList);
		CameraManager::SetCommand(cmdList, PipelineType::Object3D);
		sceneContext_->GetLightLibrary()->SetCommand(cmdList, PipelineType::Object3D);

		model->Draw(*transform);
	}

	//===================================================================*/
	// アニメーションモデル描画（視錐台カリング追加）
	//===================================================================*/
	for (const auto& [model, transform] : skinnedModels){
		if (model->GetModelData() == std::nullopt) continue;

		const Matrix4x4& worldMat = transform->matrix.world;
		AABB worldAABB = model->GetModelData()->localAABB.Transform(worldMat);

		if (!camera->IsVisible(worldAABB)){
			continue;
		}

		BlendMode mode = model->GetBlendMode();
		auto desc = PipelinePresets::MakeSkinningObject3D(mode);

		psoService->SetCommand(desc, cmdList);
		CameraManager::SetCommand(cmdList, PipelineType::SkinningObject3D);
		sceneContext_->GetLightLibrary()->SetCommand(cmdList, PipelineType::SkinningObject3D);

		model->Draw(*transform);
	}


	//===================================================================*/
	//						sprite
	//===================================================================*/
	spriteRenderer_->Draw(cmdList, psoService,renderTargetType);

	//===================================================================*/
	//                    プリミティブ描画
	//===================================================================*/
	GraphicsGroup::GetInstance()->SetCommand(cmdList, PipelineType::Line, BlendMode::NORMAL);
	CameraManager::SetCommand(cmdList, PipelineType::Line);
	PrimitiveDrawer::GetInstance()->Render();

	//===================================================================*/
	//                    particle描画
	//===================================================================*/
	sceneContext_->GetFxSystem()->Render(psoService, cmdList);
}
