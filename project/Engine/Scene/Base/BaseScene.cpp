#include "BaseScene.h"
/* ===================================================================== */
/* include space                                                         */
/* ===================================================================== */
#include "Engine/Application/System/Environment.h"

#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Assets/Model/Model.h>
#include <Engine/Assets/Model/ModelData.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/Event/BaseEventObject.h>
#include <Engine/Scene/Utility/SceneUtility.h>

BaseScene::BaseScene() {
	spriteRenderer_	 = std::make_unique<SpriteRenderer>();
	modelRenderer_	 = std::make_unique<ModelRenderer>();
	shadowMapSystem_ = std::make_unique<CalyxGraphics::ShadowMapSystem>();
	shadowMapSystem_->Initialize(
		GraphicsGroup::GetInstance()->GetDevice().Get(),
		4096);
}

void BaseScene::Initialize() {}

void BaseScene::PostUpdate(ID3D12GraphicsCommandList* cmd,
						   PipelineService*			  pso) {
	if(!sceneContext_) return;
	sceneContext_->PostUpdate(pso, cmd);
}

void BaseScene::Draw(ID3D12GraphicsCommandList* cmd,
					 PipelineService*			pso,
					 IRenderTarget*				rt) {
	if(!sceneContext_) return;

	// Skybox
	if(!skyBox_) {
		// SceneContext 経由で生成（Current に依存しない）
		skyBox_ = sceneContext_->Instantiate<SkyBox>("sky.dds", "skyBox");
		skyBox_->Initialize();
	}

	skyBox_->Draw(cmd);

	modelRenderer_->BeginFrame();

	// モデル登録
	for(auto* e : sceneContext_->GetObjectLibrary()->GetAllObjectsRaw()) {
		if(auto* go = dynamic_cast<BaseGameObject*>(e)) {
			switch(go->GetModelType()) {
			case ObjectModelType::ModelType_Static:
				if(auto* m = go->GetStaticModel())
					modelRenderer_->RegisterStatic(m, go->GetWorldTransform(), go->GetBillboardMode(), go);
				break;
			case ObjectModelType::ModelType_Animation:
				if(auto* m = go->AnimationModel())
					modelRenderer_->RegisterSkinned(m, go->GetWorldTransform(), go);
				break;
			default:
				break;
			}
		}

#if defined(_DEBUG) || defined(DEVELOP)
		// DebugView でのみ イベントも描画する
		if(rt->GetRenderTargetType() == RenderTargetType::DebugView) {
			if(auto* ev = dynamic_cast<BaseEventObject*>(e)) {
				if(auto* m = ev->GetModel()) {
					modelRenderer_->RegisterStatic(m, ev->GetWorldTransform(), BillboardMode::None, ev);
				}
			}
		}
#endif
	}

	const Camera3d* cam = static_cast<Camera3d*>(CameraManager::GetMain3d());
	modelRenderer_->PreCullAndBatch(cam);

	// =========================================================
	//  ShadowPass
	// =========================================================
	{
		auto* dirLight = sceneContext_->GetLightLibrary()->GetDirectionalLight();
		if(dirLight) {
			// シーン全体のAABBからシャドウマップの範囲を決定
			shadowMapSystem_->UpdateShadowBounds(*cam, 500.0f, 10.0f);
			dirLight->UpdateLightVP(shadowMapSystem_->GetShadowBounds().GetBounds());
			shadowMapSystem_->SetLightVP(dirLight->GetLightVP());
		}

		// ShadowMap を作る
		shadowMapSystem_->Render(
			cmd,
			pso,
			GraphicsGroup::GetInstance()->GetDevice().Get(),
			modelRenderer_->GetStaticVisible(),
			modelRenderer_->GetSkinnedVisible());
	}

	// =========================================================
	// MainPass の前に、描画先(OM)を必ず復帰させる
	// =========================================================
	{
		// RTV + DSV + Viewport を復帰
		rt->SetRenderTarget(cmd);
	}

	// =========================================================
	// MainPass
	// =========================================================
	// ===== ShadowMap を MainPass にバインド =====
	modelRenderer_->DrawAll(cmd,
							GraphicsGroup::GetInstance()->GetDevice().Get(),
							rt,
							pso,
							sceneContext_->GetLightLibrary(), shadowMapSystem_.get());

	// Particles
	sceneContext_->GetFxSystem()->Render(pso, cmd);

#if defined(_DEBUG) || defined(DEVELOP)
	// lightのデバッグ描画
	auto dirLight = sceneContext_->FindFirst<DirectionalLight>();
	if(dirLight)
		dirLight->DrawDebug();
#endif
}

void BaseScene::DrawSpritesOnly(ID3D12GraphicsCommandList* cmd,
								PipelineService*		   pso) {
	spriteRenderer_->Draw(cmd, pso, RenderTargetType::BackBuffer);
}