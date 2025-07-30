#include "SceneObjectLibrary.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/System/Event/EventBus.h>
#include <iostream>

/* 追加 ------------------------------------------------------------------*/
void SceneObjectLibrary::AddObject(const std::shared_ptr<SceneObject>& object){
	if (!object) return;
	std::cout << "[AddObject] GUID: " << object->GetGuid().ToString() << "\n";
	objects_.emplace(object->GetGuid(), object);

	EventBus::Publish(ObjectAdded {object});
}

/* 削除 ------------------------------------------------------------------*/
bool SceneObjectLibrary::RemoveObject(const std::shared_ptr<SceneObject>& obj){
	if (!obj) return false;

	std::shared_ptr<SceneObject> target;
	Guid guidToErase;

	for (const auto& [id, sp] : objects_){
		if (sp.get() == obj.get()){ //< ポインタ一致で探す
			target = sp;
			guidToErase = id;
			break;
		}
	}

	if (!target){
		std::cout << "[RemoveObject] Not found by ptr: " << obj->GetName() << "\n";
		return false;
	}

	std::cout << "[RemoveObject] Found: " << obj->GetName()
		<< ", GUID: " << guidToErase.ToString() << "\n";

	// 子も削除（省略）
	objects_.erase(guidToErase);
	return true;
}



bool SceneObjectLibrary::RemoveObject(Guid id){
	auto it = objects_.find(id);
	if (it == objects_.end()) return false;

	EventBus::Publish(ObjectRemoved {it->second});
	objects_.erase(it);
	return true;
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

bool SceneObjectLibrary::Contains(const std::shared_ptr<SceneObject>& obj) const {
	if (!obj) return false;
	return objects_.contains(obj->GetGuid());  // GUIDベースで判定
}