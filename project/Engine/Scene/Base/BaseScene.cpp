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
#include <Engine/Objects/2D/Object2d/ISpriteRenderable.h>
#include <Engine/PostProcess/Manager/PostEffectManager.h>
#include <Engine/Objects/Event/BaseEventObject.h>
#include <Engine/Scene/Utility/SceneUtility.h>

BaseScene::BaseScene() {
	spriteRenderer_	 = std::make_unique<SpriteRenderer>();
	modelRenderer_	 = std::make_unique<ModelRenderer>();
	outlineRenderer_ = std::make_unique<OutlineRenderer>();
	debugOutlineRenderer_ = std::make_unique<OutlineRenderer>();
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

	//skyBox_->Draw(cmd);

	modelRenderer_->BeginFrame();

	// モデル登録
	for(auto* e : sceneContext_->GetObjectLibrary()->GetAllObjectsRaw()) {
		if(auto* go = dynamic_cast<BaseGameObject*>(e)) {
			switch(go->GetModelType()) {
			case ObjectModelType::ModelType_Static:
				if(auto* m = go->GetStaticModel())
					modelRenderer_->RegisterStatic(m, go->GetRenderWorldTransform(), go->GetBillboardMode(), go);
				break;
			case ObjectModelType::ModelType_Animation:
				if(auto* m = go->AnimationModel())
					modelRenderer_->RegisterSkinned(m, go->GetRenderWorldTransform(), go);
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

	Camera3d* renderCam = dynamic_cast<Camera3d*>(CameraManager::GetActive());
	if(!renderCam) {
		renderCam = CameraManager::GetMain3d();
	}
	if(!renderCam) return;

	if(rt->GetRenderTargetType() == RenderTargetType::DebugView) {
		modelRenderer_->PreCullAndBatch(renderCam, false);
	} else {
		const Camera3d* cullCam = CameraManager::GetMain3d();
		if(!cullCam) {
			cullCam = renderCam;
		}
		modelRenderer_->PreCullAndBatch(cullCam, true);
	}

	// =========================================================
	// MainPass
	// =========================================================
	const bool cameraDitherEnabled = rt->GetRenderTargetType() != RenderTargetType::DebugView;
	renderCam->SetCameraDitherEnabled(cameraDitherEnabled);
	rt->SetRenderTarget(cmd);
	modelRenderer_->DrawAll(cmd,
							GraphicsGroup::GetInstance()->GetDevice().Get(),
							rt,
							pso,
							sceneContext_->GetLightLibrary(), nullptr);

	// Particles
	sceneContext_->GetFxSystem()->Render(pso, cmd);

	const bool outlineEnabled = PostEffectManager::Get()->IsOutlineEnabled();
	OutlineRenderer* outlineRenderer =
		rt->GetRenderTargetType() == RenderTargetType::DebugView ? debugOutlineRenderer_.get() : outlineRenderer_.get();

	// OutlinePass
	if(outlineEnabled && outlineRenderer) {
		outlineRenderer->Render(cmd,
								GraphicsGroup::GetInstance()->GetDevice().Get(),
								rt,
								pso,
								renderCam,
								*modelRenderer_);
	}

#if defined(_DEBUG) || defined(DEVELOP)
	if(outlineEnabled && outlineRenderer && rt->GetRenderTargetType() == RenderTargetType::DebugView) {
		outlineRenderer->RenderSelectionHighlight(cmd,
												  GraphicsGroup::GetInstance()->GetDevice().Get(),
												  rt,
												  pso,
												  renderCam,
												  *modelRenderer_,
												  sceneContext_->GetDebugSelectedObjects());
	}
#endif

#if defined(_DEBUG) || defined(DEVELOP)
	// lightのデバッグ描画
	auto dirLight = sceneContext_->FindFirst<DirectionalLight>();
	if(dirLight)
		dirLight->DrawDebug();
#endif
}

void BaseScene::DrawSpritesOnly(ID3D12GraphicsCommandList* cmd,
								PipelineService*		   pso) {
	if(sceneContext_) {
		for(auto* object : sceneContext_->GetObjectLibrary()->GetAllObjectsRaw()) {
			if(auto* spriteObject = dynamic_cast<CalyxEngine::ISpriteRenderable*>(object)) {
				// 描画登録は2Dパスごとに再構築し、SpriteRendererには所有権を渡さない。
				spriteObject->SubmitSprites(*spriteRenderer_);
			}
		}
	}
	spriteRenderer_->Draw(cmd, pso, RenderTargetType::BackBuffer);
}
