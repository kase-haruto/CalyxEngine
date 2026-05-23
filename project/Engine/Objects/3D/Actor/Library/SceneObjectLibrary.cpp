#include "SceneObjectLibrary.h"

#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/Event/Destroying/ObjectDestroying.h>
#include <Engine/System/Event/EventBus.h>
#include <iostream>
#include <map>

uint32_t SceneObjectLibrary::nextPickingID_ = 1;

SceneObjectLibrary::SceneObjectLibrary() {
	connDestroy_ = EventBus::Subscribe<ObjectDestroying>(
		[this](const ObjectDestroying& ev) {
			if(suppressDestroySync_ || !ev.object) return;

			const Guid id = ev.object->GetGuid();
			auto	   it = objects_.find(id);
			if(it == objects_.end()) return;

			EventBus::Publish(ObjectRemoved{ev.object, owner_});
			objects_.erase(it);
			RefreshDuplicateNameIndices();
		});
}
SceneObjectLibrary::~SceneObjectLibrary() = default;

namespace {
	std::string TrimName(const std::string& name) {
		const auto first = name.find_first_not_of(" \t\r\n");
		if(first == std::string::npos) return {};

		const auto last = name.find_last_not_of(" \t\r\n");
		return name.substr(first, last - first + 1);
	}

} // namespace

void SceneObjectLibrary::RefreshDuplicateNameIndices() {
	std::map<std::string, std::vector<std::shared_ptr<SceneObject>>> groups;

	for(const auto& [id, sp] : objects_) {
		(void)id;
		if(!sp) continue;
		sp->SetDuplicateNameIndex(0);
		groups[sp->GetName()].push_back(sp);
	}

	for(auto& [name, group] : groups) {
		(void)name;
		if(group.size() <= 1) continue;

		std::sort(group.begin(), group.end(),
				  [](const std::shared_ptr<SceneObject>& lhs,
					 const std::shared_ptr<SceneObject>& rhs) {
					  if(!lhs || !rhs) return lhs != nullptr;
					  return lhs->GetGuid().ToString() < rhs->GetGuid().ToString();
				  });

		for(size_t i = 0; i < group.size(); ++i) {
			if(group[i]) {
				group[i]->SetDuplicateNameIndex(static_cast<uint32_t>(i + 1));
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの追加
//////////////////////////////////////////////////////////////////////////////////
void SceneObjectLibrary::AddObject(const std::shared_ptr<SceneObject>& object) {
	if(!object) return;

	const Guid	id		  = object->GetGuid();
	if(objects_.contains(id)) return;

	std::string finalName = MakeUniqueName(object->GetName(), object.get());
	object->SetName(finalName, object->GetObjectType());

	// Picking ID 割り当て
	if(object->GetPickingID() == 0) {
		object->SetPickingID(nextPickingID_++);
	}

	// shared_ptr で登録
	objects_[id] = object;
	RefreshDuplicateNameIndices();

	// イベント発火
	EventBus::Publish(ObjectAdded{object, owner_});
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクト名の変更
//////////////////////////////////////////////////////////////////////////////////
std::string SceneObjectLibrary::RenameObject(const std::shared_ptr<SceneObject>& object,
											 const std::string&					requestedName) {
	if(!object) return {};

	const std::string finalName = MakeUniqueName(requestedName, object.get());
	object->SetName(finalName, object->GetObjectType());
	RefreshDuplicateNameIndices();
	return finalName;
}

//////////////////////////////////////////////////////////////////////////////////
///     保存用オブジェクト名として使える形に整える
//////////////////////////////////////////////////////////////////////////////////
std::string SceneObjectLibrary::MakeUniqueName(const std::string& requestedName,
											   const SceneObject* ignore) const {
	std::string baseName = TrimName(requestedName);
	if(baseName.empty()) {
		baseName = ignore ? std::string(ignore->GetObjectClassName()) : "SceneObject";
		if(baseName.empty()) baseName = "SceneObject";
	}
	return baseName;
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
	EventBus::Publish(ObjectRemoved{object, owner_});

	// DestroyRecursive で階層を断つ
	suppressDestroySync_ = true;
	object->Destroy();
	suppressDestroySync_ = false;

	// 最後にライブラリから除外
	objects_.erase(id);
	RefreshDuplicateNameIndices();
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

		suppressDestroySync_ = true;
		sp->Destroy();
		suppressDestroySync_ = false;
		EventBus::Publish(ObjectRemoved{sp, owner_});
	}

	objects_.erase(it);
	RefreshDuplicateNameIndices();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////
///     リストのクリア
//////////////////////////////////////////////////////////////////////////////////
void SceneObjectLibrary::Clear() {
	// Destroy → イベント → クリア
	for(auto& [id, sp] : objects_) {
		if(!sp) continue;
		suppressDestroySync_ = true;
		sp->Destroy();
		suppressDestroySync_ = false;
		EventBus::Publish(ObjectRemoved{sp, owner_});
	}

	objects_.clear();
	RefreshDuplicateNameIndices();
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

std::vector<std::shared_ptr<SceneObject>> SceneObjectLibrary::FindByClassName(
	std::string_view className) const {
	std::vector<std::shared_ptr<SceneObject>> result;
	for(const auto& [id, sp] : objects_) {
		(void)id;
		if(sp && sp->GetObjectClassName() == className) {
			result.push_back(sp);
		}
	}
	return result;
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
