#include "SceneContext.h"
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Scene/Runtime/IRuntimeBehaviour.h>

SceneContext* SceneContext::current_ = nullptr;

void SceneContext::Initialize(bool createDefaultLights) {
	MakeCurrent();
	objectLibrary_ = std::make_unique<SceneObjectLibrary>();
	lightLibrary_ = std::make_unique<LightLibrary>();
	fxSystem_ = std::make_unique<FxSystem>();

	if (createDefaultLights) {
		auto dir = Instantiate<DirectionalLight>("DirectionalLight");
		dir->SetEnableRaycast(false);
		auto pt = Instantiate<PointLight>("PointLight");
		pt->SetEnableRaycast(false);
		lightLibrary_->SetDirectionalLight(dir);
		lightLibrary_->SetPointLight(pt);
	}

	cameraMgr_ = std::make_unique<CameraManager>();
	cameraMgr_->Initialize(this);
}


void SceneContext::Update(float dt,bool runtimePass){
		// Runtime パス
	if (runtimePass) {
		for (auto& sp : objectLibrary_->GetAllObjectsShared()) {
			if (sp) sp->Update(dt);
		}
	}

	// Always パス
	for (auto& sp : objectLibrary_->GetAllObjectsShared()){
		if (sp) sp->AlwaysUpdate(dt);
	}

	lightLibrary_->CyncGpu();
	fxSystem_->SyncEmitters();
}

void SceneContext::RunRuntimeBootstrap() {
	// Start all
	for (auto* o : objectLibrary_->GetAllObjectsRaw())
		if (auto* b = dynamic_cast<IRuntimeBehaviour*>(o))
			b->Start();
}

void SceneContext::PostUpdate(PipelineService* psoService, ID3D12GraphicsCommandList* cmd){
	PipelineSet set = psoService->GetComputePipelineSet(PipelineTag::Compute::GpuParticle);
	set.SetCompute(cmd);
	fxSystem_->DispatchEmitters(cmd);
}

void SceneContext::Clear(){
	if (objectLibrary_){
		for (auto& sp : objectLibrary_->GetAllObjectsShared()){
			if (!sp) continue;
			if (onEditorObjectRemoved_) onEditorObjectRemoved_(sp.get());
		}
		objectLibrary_->Clear();
	}
	if (fxSystem_) fxSystem_->Clear();
	CollisionManager::GetInstance()->ClearColliders();
	PrimitiveDrawer::GetInstance()->ClearMesh();
}

void SceneContext::RemoveEditorObject(const std::shared_ptr<SceneObject>& obj){
	if (!obj) return;
	objectLibrary_->RemoveObject(obj);
	for (auto& cb : objectRemovedCallbacks_){ cb(obj.get()); }
}

std::shared_ptr<SceneObject> SceneContext::FindSharedObject(SceneObject* raw){
	for (auto& sp : objectLibrary_->GetAllObjectsShared()){
		if (sp.get() == raw) return sp;
	}
	return nullptr;
}
