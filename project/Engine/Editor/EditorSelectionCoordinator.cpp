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
		// UIとSceneEditorの所有権は各管理層に残し、選択同期先としてのみ参照する。
		hierarchy_ = hierarchy;
		inspector_ = inspector;
		sceneEditor_ = sceneEditor;
	}

	void EditorSelectionCoordinator::SetSelectedEditor(BaseEditor* editor) {
		// 専用Editor選択とSceneObject選択は排他的にし、Inspectorの表示責務を曖昧にしない。
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

		// 失効weak_ptrと重複を除いた現在集合を基準に、Ctrl選択相当の追加・解除を行う。
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

		// nullと重複を除去し、内部はLifetimeを延長しないweak_ptrとして保持する。
		std::vector<std::shared_ptr<SceneObject>> validObjects;
		validObjects.reserve(objects.size());
		for(const auto& object : objects) {
			if(!object) continue;
			if(std::find(validObjects.begin(), validObjects.end(), object) != validObjects.end()) continue;
			validObjects.push_back(object);
			selectedObjects_.push_back(object);
		}

		// 正規化した同一集合を全Consumerへ渡し、Hierarchy・Inspector・Gizmo間の不一致を防ぐ。
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

		// Editor Mode切替後のContextに存在しないObjectを選択から外し、旧Scene参照を残さない。
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

		// Snapshot中に削除されたObjectはweak_ptrのlock失敗として無視する。
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
		// 最後に追加された有効ObjectをPrimaryとし、InspectorやGizmoの代表Targetへ利用する。
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
			// SceneEditorは非所有raw pointer APIのため、Coordinator側の有効shared_ptrから都度構築する。
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
			// RendererのDebug選択表示へ同じ非所有Target集合を反映する。
			std::vector<SceneObject*> rawObjects;
			rawObjects.reserve(validObjects.size());
			for(const auto& object : validObjects) {
				rawObjects.push_back(object.get());
			}
			ctx->SetDebugSelectedObjects(rawObjects);
		}
	}

} // namespace CalyxEngine
