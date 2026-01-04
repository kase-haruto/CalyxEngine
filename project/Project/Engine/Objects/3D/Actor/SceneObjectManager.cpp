#include "SceneObjectManager.h"

SceneObjectManager::SceneObjectManager(){
	allSceneObjects_.clear();
}

SceneObjectManager* SceneObjectManager::GetInstance(){
	static SceneObjectManager instance;
	return &instance;
}

//------------------------------------------------------------------*//
//			リストのクリア
//------------------------------------------------------------------*//
void SceneObjectManager::ClearAllObject(){

	allSceneObjects_.clear();

}

void SceneObjectManager::ClearGameObjects(){
	auto it = std::remove_if(allSceneObjects_.begin(), allSceneObjects_.end(), [] (SceneObject* obj){
		return obj->GetObjectType() == ObjectType::GameObject;
							 });
	allSceneObjects_.erase(it, allSceneObjects_.end());
}

//------------------------------------------------------------------*//
//			objectの追加
//------------------------------------------------------------------*//
void SceneObjectManager::AddObject(SceneObject* object){
	allSceneObjects_.push_back(object);
}

void SceneObjectManager::RemoveObject(SceneObject* object){
	auto it = std::remove(allSceneObjects_.begin(), allSceneObjects_.end(), object);
	allSceneObjects_.erase(it, allSceneObjects_.end());	
}

void SceneObjectManager::Finalize(){
	ClearAllObject();
}