#include "SceneObjectLibrary.h"
#include <Engine/System/Event/EventBus.h>
#include <iostream>
#include <unordered_set>

/* 静的ヘルパ -------------------------------------------------------------*/
void SceneObjectLibrary::GatherSubtreePostorder(
	const std::shared_ptr<SceneObject>& node,
	std::vector<std::shared_ptr<SceneObject>>& out){
	if (!node) return;
	for (const auto& ch : node->GetChildren()){ GatherSubtreePostorder(ch,out); }
	out.push_back(node);
}

/* 追加 ------------------------------------------------------------------*/
void SceneObjectLibrary::AddObject(const std::shared_ptr<SceneObject>& object) {
	if (!object) return;
	const Guid id = object->GetGuid();

	std::string baseName = object->GetName();
	std::string finalName = baseName;

	// -------------------------
	// 名前の重複をO(1)で回避
	// -------------------------
	auto it = nameCounters_.find(baseName);
	if (it == nameCounters_.end()) {
		// 初出 → カウンタ登録
		nameCounters_[baseName] = 1;
	} else {
		// 既出 → インクリメントして "(n)" を付ける
		finalName = baseName + "(" + std::to_string(it->second++) + ")";
	}

	// 名前を更新
	object->SetName(finalName, object->GetObjectType());

	// 登録
	objects_.insert_or_assign(id, object);
	EventBus::Publish(ObjectAdded{object});
}

/* 削除（共有_ptr 指定） ---------------------------------------------------*/
bool SceneObjectLibrary::RemoveObject(const std::shared_ptr<SceneObject>& obj){
	if (!obj) return false;
	const Guid id = obj->GetGuid();
	auto it = objects_.find(id);
	if (it == objects_.end()) return false;

	RemoveSubtreePostorder(it->second);
	return true;
}

/* 削除（GUID 指定） -------------------------------------------------------*/
bool SceneObjectLibrary::RemoveObject(Guid id){
	auto it = objects_.find(id);
	if (it == objects_.end()) return false;
	RemoveSubtreePostorder(it->second);
	return true;
}

/* サブツリーを後行順で外す（イベント発火を保証） ---------------------------*/
void SceneObjectLibrary::RemoveSubtreePostorder(const std::shared_ptr<SceneObject>& root){
	std::vector<std::shared_ptr<SceneObject>> postorder;
	postorder.reserve(16);
	GatherSubtreePostorder(root,postorder);

	for (auto& node : postorder){
		if (node->GetParent()) node->SetParent(nullptr);
		auto it = objects_.find(node->GetGuid());
		if (it != objects_.end()){
			EventBus::Publish(ObjectRemoved{it->second});
			objects_.erase(it);
		}
	}
}

/* クリア（全フォレストを根から） -----------------------------------------*/
void SceneObjectLibrary::Clear(){
	if (objects_.empty()) return;

	// まず現在の全ノードをスナップショット
	std::vector<std::shared_ptr<SceneObject>> all;
	all.reserve(objects_.size());
	for (auto& [id, sp] : objects_) if (sp) all.emplace_back(sp);

	// ルート（親がいない or 親が未登録）を抽出
	std::unordered_set<Guid> live;
	live.reserve(objects_.size());
	for (auto& [id, _] : objects_) live.insert(id);

	std::vector<std::shared_ptr<SceneObject>> roots;
	roots.reserve(all.size());
	for (auto& sp : all){
		auto parent = sp->GetParent();
		const bool isRoot = !parent || !live.contains(parent->GetGuid());
		if (isRoot) roots.emplace_back(sp);
	}

	// 各ルートのサブツリーを後行順に削除（イベント発火を保証）
	for (auto& r : roots){ RemoveSubtreePostorder(r); }

	// 念のため漏れがあれば（単独ノード等）掃除
	if (!objects_.empty()){
		// 残存分もイベント付きで落とす
		std::vector<Guid> rest;
		rest.reserve(objects_.size());
		for (auto& [id, _] : objects_) rest.push_back(id);
		for (auto id : rest) RemoveObject(id);
	}
}

/* 検索 ---------------------------------------------------------------*/
std::shared_ptr<SceneObject> SceneObjectLibrary::Find(Guid id) const{
	auto it = objects_.find(id);
	return it != objects_.end() ? it->second : nullptr;
}

std::shared_ptr<SceneObject> SceneObjectLibrary::FindByName(const std::string& name) const{
	for (const auto& [id, sp] : objects_){ if (sp && sp->GetName() == name) return sp; }
	return nullptr;
}

/* 一覧取得 -------------------------------------------------------------*/
std::vector<SceneObject*> SceneObjectLibrary::GetAllObjectsRaw() const{
	std::vector<SceneObject*> result;
	result.reserve(objects_.size());
	for (const auto& [id, sp] : objects_) if (sp) result.emplace_back(sp.get());
	return result;
}

std::vector<std::shared_ptr<SceneObject>> SceneObjectLibrary::GetAllObjectsShared() const{
	std::vector<std::shared_ptr<SceneObject>> result;
	result.reserve(objects_.size());
	for (const auto& [id, sp] : objects_) result.emplace_back(sp);
	return result;
}

bool SceneObjectLibrary::Contains(const std::shared_ptr<SceneObject>& obj) const{
	if (!obj) return false;
	return objects_.contains(obj->GetGuid());
}