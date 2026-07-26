#pragma once

#include <memory>
#include <cstdint>
#include <unordered_map>
#include <vector>

class SceneObject;
class SceneObjectLibrary;

namespace CalyxEngine {

	/**
	 * @brief HierarchyTreeCacheの機能を提供するクラスです。
	 */
	class HierarchyTreeCache {
	public:
		void MarkDirty() { dirty_ = true; }
		void Clear();

		const std::vector<std::shared_ptr<SceneObject>>& GetRoots(const SceneObjectLibrary& library);
		const std::vector<std::shared_ptr<SceneObject>>& GetChildren(SceneObject& object);

	private:
		void InvalidateIfLibraryChanged(const SceneObjectLibrary& library);

		std::unordered_map<const SceneObject*, std::vector<std::shared_ptr<SceneObject>>> sortedChildren_;
		const SceneObjectLibrary* cachedLibrary_ = nullptr;
		uint64_t cachedRevision_ = 0;
		bool dirty_ = true;
	};

} // namespace CalyxEngine
