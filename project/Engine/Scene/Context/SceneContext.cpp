#include "SceneContext.h"
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

SceneContext::SceneContext(){

}

SceneContext::~SceneContext() = default;

void SceneContext::Initialize(){
	editorObjects_.clear();
	objectLibrary_ = std::make_unique<SceneObjectLibrary>();
	lightLibrary_ = std::make_unique<LightLibrary>(objectLibrary_.get());
	fxSystem_ = std::make_unique<FxSystem>();
}

void SceneContext::Update(){
	for (auto& weakObj : editorObjects_){
		if (auto obj = weakObj.lock()){
			obj->Update();
		}
	}

	lightLibrary_->Update();
	fxSystem_->Update();
}

void SceneContext::Clear(){
	if (objectLibrary_){
		const auto& objects = objectLibrary_->GetAllObjects();
		for (const auto& obj : objects){
			if (onEditorObjectRemoved_){
				onEditorObjectRemoved_(obj);
			}
		}
		objectLibrary_->Clear();
	}

	editorObjects_.clear();

	fxSystem_->Clear();
	CollisionManager::GetInstance()->ClearColliders();
	PrimitiveDrawer::GetInstance()->ClearMesh();
}

void SceneContext::RemoveEditorObject(const std::shared_ptr<SceneObject>& obj){
	objectLibrary_->RemoveObject(obj);

	editorObjects_.erase(
		std::remove_if(
		editorObjects_.begin(), editorObjects_.end(),
		[&obj] (const std::weak_ptr<SceneObject>& w){
			return !w.owner_before(obj) && !obj.owner_before(w.lock());
		}
	),
		editorObjects_.end()
	);

	for (auto& cb : objectRemovedCallbacks_){
		cb(obj.get());
	}
}


std::vector<std::shared_ptr<SceneObject>> SceneContext::GetEditorObjects(){
	std::vector<std::shared_ptr<SceneObject>> validObjects;

	for (const auto& w : editorObjects_){
		if (auto locked = w.lock()){
			validObjects.push_back(locked);
		}
	}

	return validObjects;
}

std::shared_ptr<SceneObject> SceneContext::FindSharedObject(SceneObject* ptr){
	for (const auto& s : objectLibrary_->GetAllObjectsShared()){
		if (s.get() == ptr){
			return s;
		}
	}
	return nullptr;
}