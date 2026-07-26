#pragma once

#include <memory>
#include <vector>

class SceneObject;
class SceneContext;

namespace CalyxEngine {

	class BaseEditor;
	class HierarchyPanel;
	class InspectorPanel;
	class SceneObjectEditor;

	/**
	 * @brief EditorSelectionCoordinatorの機能を提供するクラスです。
	 */
	class EditorSelectionCoordinator {
	public:
		/**
		 * @brief Snapshotに関するデータを保持する構造体です。
		 */
		struct Snapshot {
			BaseEditor* selectedEditor = nullptr;
			std::vector<std::weak_ptr<SceneObject>> selectedObjects;
		};

		void Bind(HierarchyPanel* hierarchy,
				  InspectorPanel* inspector,
				  SceneObjectEditor* sceneEditor);

		void SetSelectedEditor(BaseEditor* editor);
		void SetSelectedObject(const std::shared_ptr<SceneObject>& object);
		void ToggleSelectedObject(const std::shared_ptr<SceneObject>& object);
		void SetSelectedObjects(const std::vector<std::shared_ptr<SceneObject>>& objects);
		void Clear();
		void ClearSceneContextSelection();
		void PruneToContext(SceneContext* context);
		Snapshot Capture() const;
		void Restore(const Snapshot& snapshot);

		bool HasSelection() const;
		bool IsSelected(const SceneObject* object) const;
		std::shared_ptr<SceneObject> GetPrimarySelectedObject() const;
		std::vector<std::shared_ptr<SceneObject>> GetSelectedObjects() const;

	private:
		void SyncPanels(const std::vector<std::shared_ptr<SceneObject>>& validObjects);
		void SyncSceneContext(const std::vector<std::shared_ptr<SceneObject>>& validObjects);

		HierarchyPanel* hierarchy_ = nullptr;
		InspectorPanel* inspector_ = nullptr;
		SceneObjectEditor* sceneEditor_ = nullptr;
		BaseEditor* selectedEditor_ = nullptr;
		std::vector<std::weak_ptr<SceneObject>> selectedObjects_;
	};

} // namespace CalyxEngine
