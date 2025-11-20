#include "SceneObjectManager.h"

SceneObjectManager::SceneObjectManager(){
	allSceneObjects_.clear();
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

void SceneObjectManager::Finalize(){
	ClearAllObject();
}