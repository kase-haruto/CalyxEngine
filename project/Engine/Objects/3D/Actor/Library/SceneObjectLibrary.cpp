#include "SceneObjectLibrary.h"

#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/System/Event/EventBus.h>
#include <iostream>

uint32_t SceneObjectLibrary::nextPickingID_ = 1;

SceneObjectLibrary::SceneObjectLibrary()  = default;
SceneObjectLibrary::~SceneObjectLibrary() = default;

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの追加
//////////////////////////////////////////////////////////////////////////////////
void SceneObjectLibrary::AddObject(const std::shared_ptr<SceneObject>& object) {
	if(!object) return;

	const Guid	id		  = object->GetGuid();
	std::string baseName  = object->GetName();
	std::string finalName = baseName;

	// -----------------------------------------
	// 名前重複回避
	// -----------------------------------------
	auto it = nameCounters_.find(baseName);
	if(it == nameCounters_.end()) {
		nameCounters_[baseName] = 1;
	} else {
		finalName = baseName + "(" + std::to_string(it->second++) + ")";
	}

	object->SetName(finalName, object->GetObjectType());

	// Picking ID 割り当て
	if(object->GetPickingID() == 0) {
		object->SetPickingID(nextPickingID_++);
	}

	// shared_ptr で登録
	objects_[id] = object;

	// イベント発火
	EventBus::Publish(ObjectAdded{object});
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの削除(shared_ptr指定)
//////////////////////////////////////////////////////////////////////////////////
bool SceneObjectLibrary::RemoveObject(const std::shared_ptr<SceneObject>& object) {
	if(!object) return false;
	Guid id = object->GetGuid();
	std::cout << "[REMOVE] " << object->GetName()
			  << " GUID=" << id.ToString()
			  << " use_count=" << object.use_count() << std::endl;

	// 子を完全に削除（再帰）
	auto children = object->GetChildren();
	for(auto& child : children) {
		if(child) {
			RemoveObject(child);
		}
	}

	// 先に削除イベントを発火（FxSystem が emitter を消す）
	EventBus::Publish(ObjectRemoved{object});

	// DestroyRecursive で階層を断つ
	object->Destroy();

	// 最後にライブラリから除外
	objects_.erase(id);
	std::cout << "[AFTER ERASE]"
			  << " use_count=" << object.use_count()
			  << std::endl;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////
///     idをもとに削除
//////////////////////////////////////////////////////////////////////////////////
bool SceneObjectLibrary::RemoveObject(Guid id) {
	auto it = objects_.find(id);
	if(it == objects_.end()) return false;

	if(auto sp = it->second) {
		// 子リストをコピーしてから再帰削除
		auto children = sp->GetChildren();
		for(auto& child : children) {
			if(child) {
				RemoveObject(child);
			}
		}

		sp->Destroy();
		EventBus::Publish(ObjectRemoved{sp});
	}

	objects_.erase(it);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////
///     リストのクリア
//////////////////////////////////////////////////////////////////////////////////
void SceneObjectLibrary::Clear() {
	// Destroy → イベント → クリア
	for(auto& [id, sp] : objects_) {
		if(!sp) continue;
		sp->Destroy();
		EventBus::Publish(ObjectRemoved{sp});
	}

	objects_.clear();
	nameCounters_.clear(); // ここは好み。リセットしたいなら消す
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの検索(idから)
//////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<SceneObject> SceneObjectLibrary::Find(Guid id) const {
	auto it = objects_.find(id);
	if(it == objects_.end()) return nullptr;
	return it->second;
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの検索(名前から)
//////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<SceneObject> SceneObjectLibrary::FindByName(const std::string& name) const {
	for(const auto& [id, sp] : objects_) {
		if(sp && sp->GetName() == name) {
			return sp;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの一覧取得(raw)
//////////////////////////////////////////////////////////////////////////////////
std::vector<SceneObject*> SceneObjectLibrary::GetAllObjectsRaw() const {
	std::vector<SceneObject*> result;
	result.reserve(objects_.size());

	for(const auto& [id, sp] : objects_) {
		if(sp) {
			result.push_back(sp.get());
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの一覧取得(shared_ptr版)
//////////////////////////////////////////////////////////////////////////////////
std::vector<std::shared_ptr<SceneObject>> SceneObjectLibrary::GetAllObjectsShared() const {
	std::vector<std::shared_ptr<SceneObject>> result;
	result.reserve(objects_.size());

	for(const auto& [id, sp] : objects_) {
		if(sp) {
			result.push_back(sp);
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの所属判定
//////////////////////////////////////////////////////////////////////////////////
bool SceneObjectLibrary::Contains(const std::shared_ptr<SceneObject>& obj) const {
	if(!obj) return false;
	return objects_.contains(obj->GetGuid());
}

namespace {
	// シェーダー(picking.ps)と同一のハッシュ関数
	// ピッキング結果の可視化のために色を分散させているため、検索時もこれを通す必要がある
	uint32_t Hash(uint32_t x) {
		x ^= x >> 17;
		x *= 0xed5ad4bb;
		x ^= x >> 11;
		x *= 0xac4c1b51;
		x ^= x >> 15;
		x *= 0x31848bab;
		x ^= x >> 14;
		return x;
	}
} // namespace

std::shared_ptr<SceneObject> SceneObjectLibrary::FindSharedByPickingID(uint32_t hashedPickingID) const {
	for(const auto& [id, sp] : objects_) {
		// シェーダーがRGB(24bit)に書き出しているため、検索時も下位24bitのみで比較する
		if(sp && (Hash(sp->GetPickingID()) & 0x00FFFFFF) == hashedPickingID) {
			return sp;
		}
	}
	return nullptr;
}
