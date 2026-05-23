#include "EditorSelectionCoordinator.h"

#include <Engine/Application/UI/Panels/HierarchyPanel.h>
#include <Engine/Application/UI/Panels/InspectorPanel.h>
#include <Engine/Editor/BaseEditor.h>
#include <Engine/Editor/SceneObjectEditor.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <algorithm>

namespace CalyxEngine {

	void EditorSelectionCoordinator::Bind(HierarchyPanel* hierarchy,
										  InspectorPanel* inspector,
										  SceneObjectEditor* sceneEditor) {
		hierarchy_ = hierarchy;
		inspector_ = inspector;
		sceneEditor_ = sceneEditor;
	}

	void EditorSelectionCoordinator::SetSelectedEditor(BaseEditor* editor) {
		selectedEditor_ = editor;
		selectedObjects_.clear();

		if(inspector_) {
			inspector_->SetSelectedEditor(editor);
			inspector_->SetSelectedObject(std::shared_ptr<SceneObject>{});
		}
		if(sceneEditor_) {
			sceneEditor_->ClearSelection();
		}
		if(auto* ctx = SceneContext::Current()) {
			ctx->SetDebugSelectedObjects({});
		}
	}

	void EditorSelectionCoordinator::SetSelectedObject(const std::shared_ptr<SceneObject>& object) {
		if(object) {
			SetSelectedObjects({object});
		} else {
			SetSelectedObjects({});
		}
	}

	void EditorSelectionCoordinator::ToggleSelectedObject(const std::shared_ptr<SceneObject>& object) {
		if(!object) {
			Clear();
			return;
		}

		auto objects = GetSelectedObjects();
		auto it = std::find(objects.begin(), objects.end(), object);
		if(it != objects.end()) {
			objects.erase(it);
		} else {
			objects.push_back(object);
		}
		SetSelectedObjects(objects);
	}

	void EditorSelectionCoordinator::SetSelectedObjects(const std::vector<std::shared_ptr<SceneObject>>& objects) {
		selectedObjects_.clear();
		selectedEditor_ = nullptr;

		std::vector<std::shared_ptr<SceneObject>> validObjects;
		validObjects.reserve(objects.size());
		for(const auto& object : objects) {
			if(!object) continue;
			if(std::find(validObjects.begin(), validObjects.end(), object) != validObjects.end()) continue;
			validObjects.push_back(object);
			selectedObjects_.push_back(object);
		}

		SyncPanels(validObjects);
		SyncSceneContext(validObjects);
	}

	void EditorSelectionCoordinator::Clear() {
		SetSelectedObjects({});
	}

	void EditorSelectionCoordinator::ClearSceneContextSelection() {
		if(auto* ctx = SceneContext::Current()) {
			ctx->SetDebugSelectedObjects({});
		}
	}

	void EditorSelectionCoordinator::PruneToContext(SceneContext* context) {
		if(!context || !context->GetObjectLibrary()) {
			Clear();
			return;
		}

		std::vector<std::shared_ptr<SceneObject>> validObjects;
		for(const auto& object : GetSelectedObjects()) {
			if(object && context->GetObjectLibrary()->Contains(object)) {
				validObjects.push_back(object);
			}
		}
		SetSelectedObjects(validObjects);
	}

	EditorSelectionCoordinator::Snapshot EditorSelectionCoordinator::Capture() const {
		Snapshot snapshot;
		snapshot.selectedEditor = selectedEditor_;
		snapshot.selectedObjects = selectedObjects_;
		return snapshot;
	}

	void EditorSelectionCoordinator::Restore(const Snapshot& snapshot) {
		if(snapshot.selectedEditor) {
			SetSelectedEditor(snapshot.selectedEditor);
			return;
		}

		std::vector<std::shared_ptr<SceneObject>> validObjects;
		validObjects.reserve(snapshot.selectedObjects.size());
		for(const auto& weak : snapshot.selectedObjects) {
			if(auto object = weak.lock()) {
				validObjects.push_back(object);
			}
		}
		SetSelectedObjects(validObjects);
	}

	bool EditorSelectionCoordinator::HasSelection() const {
		for(const auto& weak : selectedObjects_) {
			if(weak.lock()) return true;
		}
		return false;
	}

	bool EditorSelectionCoordinator::IsSelected(const SceneObject* object) const {
		if(!object) return false;
		for(const auto& weak : selectedObjects_) {
			auto selected = weak.lock();
			if(selected.get() == object) return true;
		}
		return false;
	}

	std::shared_ptr<SceneObject> EditorSelectionCoordinator::GetPrimarySelectedObject() const {
		for(auto it = selectedObjects_.rbegin(); it != selectedObjects_.rend(); ++it) {
			if(auto selected = it->lock()) {
				return selected;
			}
		}
		return nullptr;
	}

	std::vector<std::shared_ptr<SceneObject>> EditorSelectionCoordinator::GetSelectedObjects() const {
		std::vector<std::shared_ptr<SceneObject>> objects;
		objects.reserve(selectedObjects_.size());
		for(const auto& weak : selectedObjects_) {
			auto object = weak.lock();
			if(!object) continue;
			if(std::find(objects.begin(), objects.end(), object) != objects.end()) continue;
			objects.push_back(object);
		}
		return objects;
	}

	void EditorSelectionCoordinator::SyncPanels(const std::vector<std::shared_ptr<SceneObject>>& validObjects) {
		if(hierarchy_) {
			hierarchy_->SetSelectedObjects(validObjects);
		}
		if(inspector_) {
			inspector_->SetSelectedEditor(nullptr);
			inspector_->SetSelectedObjects(selectedObjects_);
		}
		if(sceneEditor_) {
			std::vector<SceneObject*> rawObjects;
			rawObjects.reserve(validObjects.size());
			for(const auto& object : validObjects) {
				rawObjects.push_back(object.get());
			}
			sceneEditor_->SetTargets(rawObjects);
		}
	}

	void EditorSelectionCoordinator::SyncSceneContext(const std::vector<std::shared_ptr<SceneObject>>& validObjects) {
		if(auto* ctx = SceneContext::Current()) {
			std::vector<SceneObject*> rawObjects;
			rawObjects.reserve(validObjects.size());
			for(const auto& object : validObjects) {
				rawObjects.push_back(object.get());
			}
			ctx->SetDebugSelectedObjects(rawObjects);
		}
	}

} // namespace CalyxEngine
