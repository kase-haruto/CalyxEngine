#include "HierarchyTreeCache.h"

#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>

#include <algorithm>

namespace CalyxEngine {

	namespace {
		int TypePriority(ObjectType type) {
			switch(type) {
			case ObjectType::Camera:
				return 0;
			case ObjectType::Light:
				return 1;
			case ObjectType::GameObject:
				return 2;
			case ObjectType::Effect:
				return 3;
			case ObjectType::Event:
				return 4;
			default:
				return 9;
			}
		}

		bool LessByTypeThenName(const std::shared_ptr<SceneObject>& a,
								const std::shared_ptr<SceneObject>& b) {
			const int pa = TypePriority(a->GetObjectType());
			const int pb = TypePriority(b->GetObjectType());
			if(pa != pb) return pa < pb;
			return a->GetName() < b->GetName();
		}
	} // namespace

	void HierarchyTreeCache::Clear() {
		sortedChildren_.clear();
		dirty_ = true;
	}

	void HierarchyTreeCache::InvalidateIfLibraryChanged(const SceneObjectLibrary& library) {
		const uint64_t revision = library.GetRevision();
		if(cachedLibrary_ == &library && cachedRevision_ == revision) {
			return;
		}

		sortedChildren_.clear();
		cachedLibrary_ = &library;
		cachedRevision_ = revision;
		dirty_ = false;
	}

	const std::vector<std::shared_ptr<SceneObject>>& HierarchyTreeCache::GetRoots(
		const SceneObjectLibrary& library) {
		InvalidateIfLibraryChanged(library);

		if(dirty_) {
			sortedChildren_.clear();
			dirty_ = false;
			cachedLibrary_ = &library;
			cachedRevision_ = library.GetRevision();
		}

		auto it = sortedChildren_.find(nullptr);
		if(it != sortedChildren_.end()) {
			return it->second;
		}

		std::vector<std::shared_ptr<SceneObject>> roots;
		const auto& objects = library.GetObjects();
		roots.reserve(objects.size());

		for(const auto& [id, object] : objects) {
			(void)id;
			if(!object || object->IsTransient()) continue;
			auto parent = object->GetParent();
			if(!parent || !library.Contains(parent)) {
				roots.push_back(object);
			}
		}

		std::sort(roots.begin(), roots.end(), LessByTypeThenName);
		auto [insertedIt, inserted] = sortedChildren_.emplace(nullptr, std::move(roots));
		(void)inserted;
		return insertedIt->second;
	}

	const std::vector<std::shared_ptr<SceneObject>>& HierarchyTreeCache::GetChildren(
		SceneObject& object) {
		auto it = sortedChildren_.find(&object);
		if(it != sortedChildren_.end()) {
			return it->second;
		}

		std::vector<std::shared_ptr<SceneObject>> children;
		for(auto& child : object.GetChildren()) {
			if(child && !child->IsTransient()) {
				children.push_back(child);
			}
		}
		std::sort(children.begin(), children.end(), LessByTypeThenName);

		auto [insertedIt, inserted] = sortedChildren_.emplace(&object, std::move(children));
		(void)inserted;
		return insertedIt->second;
	}

} // namespace CalyxEngine
