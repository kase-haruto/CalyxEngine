#pragma once

#include <Engine/System/Command/EditorCommand/BaseLevelEditorCommand.h>
#include <Engine/Editor/SceneObjectDuplicator.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

class SceneContext;
class SceneObject;

namespace CalyxEngine {

	struct SceneObjectEditCommandCallbacks {
		std::function<void()> refreshHierarchy;
		std::function<void()> clearSelection;
		std::function<void(const std::vector<std::shared_ptr<SceneObject>>&)> selectObjects;
	};

	std::unique_ptr<ICommand> CreateDeleteSceneObjectsCommand(
		SceneContext* ctx,
		std::vector<std::shared_ptr<SceneObject>> targets,
		SceneObjectEditCommandCallbacks callbacks,
		std::string name);

	class DuplicateSceneObjectsCommand final
		: public BaseLevelEditorCommand {
	public:
		DuplicateSceneObjectsCommand(SceneContext* ctx,
									 std::vector<std::shared_ptr<SceneObject>> sources,
									 SceneObjectEditCommandCallbacks callbacks);

		void Execute() override;
		void Undo() override;
		void Redo() override;

		const std::vector<std::shared_ptr<SceneObject>>& GetCreatedRoots() const;

	private:
		SceneObjectDuplicateResult CreateDuplicates();

		SceneContext* ctx_ = nullptr;
		std::vector<Guid> sourceGuids_;
		SceneObjectDuplicateResult result_;
		SceneObjectEditCommandCallbacks callbacks_;
	};

} // namespace CalyxEngine
