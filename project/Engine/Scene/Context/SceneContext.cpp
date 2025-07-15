#include "SceneContext.h"
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

void SceneContext::Initialize(){
	objectLibrary_ = std::make_unique<SceneObjectLibrary>();
	lightLibrary_ = std::make_unique<LightLibrary>(objectLibrary_.get());
	fxSystem_ = std::make_unique<FxSystem>();

	skyBox_ = std::make_unique<SkyBox>("sky.dds", "skyBox");
	skyBox_->Initialize();

	spriteRenderer_ = std::make_unique<SpriteRenderer>();
	modelRenderer_ = std::make_unique<ModelRenderer>();
}

// ---------------------------------------------------------------------------
void SceneContext::Update(){
	for (auto& sp : objectLibrary_->GetAllObjectsShared()){
		if (sp) sp->Update();
	}

	lightLibrary_->CyncGpu();
	fxSystem_->SyncEmitters();
}

void SceneContext::Render(ID3D12GraphicsCommandList* cmdList,
						  PipelineService* psoService,
						  RenderTargetType rtType){
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

	for (auto* entry : GetObjectLibrary()->GetAllObjects()){
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
							GetLightLibrary());


	//===================================================================*/
	//						sprite
	//===================================================================*/
	spriteRenderer_->Draw(cmdList, psoService, rtType);

	//===================================================================*/
	//                    particle描画
	//===================================================================*/

	GetFxSystem()->Render(psoService, cmdList);
}

// ---------------------------------------------------------------------------
void SceneContext::Clear(){
	if (objectLibrary_){
		for (auto& sp : objectLibrary_->GetAllObjectsShared()){
			if (!sp) continue;
			if (onEditorObjectRemoved_) onEditorObjectRemoved_(sp.get());
		}
		objectLibrary_->Clear();
	}

	if (fxSystem_)  fxSystem_->Clear();
	CollisionManager::GetInstance()->ClearColliders();
	PrimitiveDrawer::GetInstance()->ClearMesh();
}

// ---------------------------------------------------------------------------
void SceneContext::RemoveEditorObject(const std::shared_ptr<SceneObject>& obj){
	if (!obj) return;

	objectLibrary_->RemoveObject(obj);

	for (auto& cb : objectRemovedCallbacks_){
		cb(obj.get());
	}
}

// ---------------------------------------------------------------------------
std::shared_ptr<SceneObject> SceneContext::FindSharedObject(SceneObject* raw){
	for (auto& sp : objectLibrary_->GetAllObjectsShared()){
		if (sp.get() == raw) return sp;
	}
	return nullptr;
}
