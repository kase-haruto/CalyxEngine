#pragma once

#include <Engine/Foundation/Utility/Guid/Guid.h>

#include <memory>
#include <vector>

class SceneContext;
class SceneObject;

namespace CalyxEngine {

	struct SceneObjectDuplicateResult {
		std::vector<std::shared_ptr<SceneObject>> selectedRoots;
		std::vector<Guid> rootGuids;
	};

	class SceneObjectDuplicator {
	public:
		static bool IsDuplicatable(const SceneObject* object);
		static std::vector<std::shared_ptr<SceneObject>> FilterDuplicatable(
			const std::vector<std::shared_ptr<SceneObject>>& objects);
		static SceneObjectDuplicateResult Duplicate(
			SceneContext* ctx,
			const std::vector<std::shared_ptr<SceneObject>>& sources);
	};

} // namespace CalyxEngine
