#include "SceneObjectLibrary.h"
#include <Engine/System/Event/EventBus.h>
#include <iostream>

static void GatherSubtreePostorder(
	const std::shared_ptr<SceneObject>& node,
	std::vector<std::shared_ptr<SceneObject>>& out)
{
	if (!node) return;
	// まず全ての子を収集
	for (const auto& ch : node->GetChildren()) { // GetChildren はあります
		GatherSubtreePostorder(ch, out);
	}
	// 最後に自分
	out.push_back(node);
}

/* 追加 ------------------------------------------------------------------*/
void SceneObjectLibrary::AddObject(const std::shared_ptr<SceneObject>& object){
	if (!object) return;
	std::cout << "[AddObject] GUID: " << object->GetGuid().ToString() << "\n";
	objects_.emplace(object->GetGuid(), object);

	EventBus::Publish(ObjectAdded {object});
}

/* 削除 ------------------------------------------------------------------*/
	bool SceneObjectLibrary::RemoveObject(const std::shared_ptr<SceneObject>& obj) {
		if (!obj) return false;

		std::shared_ptr<SceneObject> target;
		for (const auto& [id, sp] : objects_) {
			if (sp.get() == obj.get()) {
				target = sp;
				break;
			}
		}
		if (!target) {
			std::cout << "[RemoveObject] Not found by ptr: " 
					  << (obj ? obj->GetName() : "null") << "\n";
			return false;
		}

		// サブツリーを後置順で収集
		std::vector<std::shared_ptr<SceneObject>> postorder;
		GatherSubtreePostorder(target, postorder);

		// 子から順に削除
		for (auto& node : postorder) {
			// 親リンクを切る
			if (auto parent = node->GetParent()) {
				node->SetParent(nullptr);
			}

			auto it = objects_.find(node->GetGuid());
			if (it != objects_.end()) {
				EventBus::Publish(ObjectRemoved{it->second});
				std::cout << "[RemoveObject] " << node->GetName()
						  << " GUID=" << node->GetGuid().ToString() << "\n";
				objects_.erase(it);
			}
		}

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