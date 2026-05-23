#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

class SceneObject;
class SceneObjectLibrary;

namespace CalyxEngine {

	class HierarchyTreeCache {
	public:
		void MarkDirty() { dirty_ = true; }
		void Clear();

		const std::vector<std::shared_ptr<SceneObject>>& GetRoots(const SceneObjectLibrary& library);
		const std::vector<std::shared_ptr<SceneObject>>& GetChildren(SceneObject& object);

	private:
		std::unordered_map<const SceneObject*, std::vector<std::shared_ptr<SceneObject>>> sortedChildren_;
		bool dirty_ = true;
	};

} // namespace CalyxEngine
