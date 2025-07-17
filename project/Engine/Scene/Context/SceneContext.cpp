#include "SceneContext.h"
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

SceneContext* SceneContext::current_ = nullptr;

void SceneContext::Initialize(){
	objectLibrary_ = std::make_unique<SceneObjectLibrary>();
	lightLibrary_ = std::make_unique<LightLibrary>();
	fxSystem_ = std::make_unique<FxSystem>();

	// ライトを生成して登録
	auto dirLight = Instantiate<DirectionalLight>("DirectionalLight");
	auto pointLight = Instantiate<PointLight>("PointLight");

	lightLibrary_->SetDirectionalLight(dirLight);
	lightLibrary_->SetPointLight(pointLight);

}

// ---------------------------------------------------------------------------
void SceneContext::Update(){
	for (auto& sp : objectLibrary_->GetAllObjectsShared()){
		if (sp) sp->Update();
	}

	lightLibrary_->CyncGpu();
	fxSystem_->SyncEmitters();
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
