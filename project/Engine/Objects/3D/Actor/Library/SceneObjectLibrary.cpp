#include "SceneObjectLibrary.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

/* 追加 ------------------------------------------------------------------*/
void SceneObjectLibrary::AddObject(const std::shared_ptr<SceneObject>& object){
	if (!object) return;
	objects_.emplace(object->GetGuid(), object);
}

/* 削除 (shared_ptr 版) --------------------------------------------------*/
bool SceneObjectLibrary::RemoveObject(const std::shared_ptr<SceneObject>& object){
	if (!object) return false;
	return RemoveObject(object->GetGuid());
}

/* 削除 (Guid 版) --------------------------------------------------------*/
bool SceneObjectLibrary::RemoveObject(Guid id){
	if (!id.isValid()) return false;
	return objects_.erase(id) > 0;
}

/* クリア ---------------------------------------------------------------*/
void SceneObjectLibrary::Clear(){
	objects_.clear();
}

/* 検索 ---------------------------------------------------------------*/
std::shared_ptr<SceneObject> SceneObjectLibrary::Find(Guid id) const{
	auto it = objects_.find(id);
	return it != objects_.end() ? it->second : nullptr;
}

std::shared_ptr<SceneObject> SceneObjectLibrary::FindByName(const std::string& name) const{
	for (const auto& [id, sp] : objects_){
		if (sp && sp->GetName() == name) return sp;
	}
	return nullptr;
}

/* 一覧取得 -------------------------------------------------------------*/
std::vector<SceneObject*> SceneObjectLibrary::GetAllObjectsRaw() const{
	std::vector<SceneObject*> result;
	result.reserve(objects_.size());
	for (const auto& [id, sp] : objects_){
		if (sp) result.emplace_back(sp.get());
	}
	return result;
}

std::vector<std::shared_ptr<SceneObject>> SceneObjectLibrary::GetAllObjectsShared() const{
	std::vector<std::shared_ptr<SceneObject>> result;
	result.reserve(objects_.size());
	for (const auto& [id, sp] : objects_){
		result.emplace_back(sp);
	}
	return result;
}
