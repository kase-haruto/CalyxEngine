#include "SceneObjectLibrary.h"

void SceneObjectLibrary::AddObject(const std::shared_ptr<SceneObject>& object){
	if (object){
		allSceneObjects_.push_back(object);
	}
}

void SceneObjectLibrary::RemoveObject(const std::shared_ptr<SceneObject>& obj){
	if (!obj) return;
	allSceneObjects_.erase(
		std::remove(allSceneObjects_.begin(), allSceneObjects_.end(), obj),
		allSceneObjects_.end()
	);
}

void SceneObjectLibrary::Clear(){
	allSceneObjects_.clear();
}

std::vector<SceneObject*> SceneObjectLibrary::GetAllObjects() const{
	std::vector<SceneObject*> rawPtrs;
	rawPtrs.reserve(allSceneObjects_.size());
	for (const auto& obj : allSceneObjects_){
		if (obj) rawPtrs.push_back(obj.get());
	}
	return rawPtrs;
}
