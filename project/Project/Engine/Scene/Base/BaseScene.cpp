#include "BaseScene.h"
/* ===================================================================== */
/* include space                                                         */
/* ===================================================================== */
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Assets/Model/Model.h>
#include <Engine/Assets/Model/ModelData.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Scene/Utility/SceneUtility.h>

BaseScene::BaseScene() {
    spriteRenderer_ = std::make_unique<SpriteRenderer>();
    modelRenderer_  = std::make_unique<ModelRenderer>();
}

void BaseScene::Initialize() {
    // 必要なら各派生で実装
}

void BaseScene::PostUpdate(ID3D12GraphicsCommandList* cmd,
                           PipelineService* pso) {
    if (!sceneContext_) return;
    sceneContext_->PostUpdate(pso, cmd);
}

void BaseScene::Draw(ID3D12GraphicsCommandList* cmd,
                     PipelineService* pso,
                     RenderTargetType) {
    if (!sceneContext_) return;

    // Skybox
    if (!skyBox_) {
        // SceneContext 経由で生成（Current に依存しない）
        skyBox_ = sceneContext_->Instantiate<SkyBox>("sky.dds", "skyBox");
        skyBox_->Initialize();
    }

    cmd->SetGraphicsRootSignature(
        GraphicsGroup::GetInstance()->GetRootSignature(PipelineType::Skybox).Get());
    skyBox_->Draw(cmd);

    modelRenderer_->BeginFrame();

    // モデル登録
    for (auto* e : sceneContext_->GetObjectLibrary()->GetAllObjectsRaw()) {
        if (auto* go = dynamic_cast<BaseGameObject*>(e)) {
            switch (go->GetModelType()) {
                case ObjectModelType::ModelType_Static:
                    if (auto* m = go->GetStaticModel())
                        modelRenderer_->RegisterStatic(m, go->GetWorldTransform(),go->GetBillboardMode());
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
                            sceneContext_->GetLightLibrary());

    // Particles
    sceneContext_->GetFxSystem()->Render(pso, cmd);
}

void BaseScene::DrawSpritesOnly(ID3D12GraphicsCommandList* cmd,
                                PipelineService* pso) {
    spriteRenderer_->Draw(cmd, pso, RenderTargetType::BackBuffer);
}

