#pragma once

#include <functional>
#include <memory>
#include <string>

class SceneObject;

namespace CalyxEngine {

	class IHierarchyActions {
	public:
		virtual ~IHierarchyActions() = default;

		virtual void SelectObject(std::shared_ptr<SceneObject> object, bool toggle) = 0;
		virtual void DeleteObject(std::shared_ptr<SceneObject> object) = 0;
		virtual void CreateObject(std::shared_ptr<SceneObject> object) = 0;
		virtual void RenameObject(std::shared_ptr<SceneObject> object, const std::string& newName) = 0;
		virtual void ApplyPrefabOverrides(std::shared_ptr<SceneObject> object) = 0;
		virtual void FocusObject(std::shared_ptr<SceneObject> object) = 0;
	};

	class CallbackHierarchyActions final
		: public IHierarchyActions {
	public:
		using SelectCB = std::function<void(std::shared_ptr<SceneObject>, bool toggle)>;
		using ObjectCB = std::function<void(std::shared_ptr<SceneObject>)>;
		using RenameCB = std::function<void(std::shared_ptr<SceneObject>, const std::string& newName)>;

		void SetOnObjectSelected(SelectCB cb) { onSelect_ = std::move(cb); }
		void SetOnObjectDelete(ObjectCB cb) { onDelete_ = std::move(cb); }
		void SetOnObjectCreate(ObjectCB cb) { onCreate_ = std::move(cb); }
		void SetOnObjectRename(RenameCB cb) { onRename_ = std::move(cb); }
		void SetOnApplyPrefabOverrides(ObjectCB cb) { onApplyPrefab_ = std::move(cb); }
		void SetOnObjectFocused(ObjectCB cb) { onFocus_ = std::move(cb); }

		void SelectObject(std::shared_ptr<SceneObject> object, bool toggle) override {
			if(onSelect_) onSelect_(std::move(object), toggle);
		}

		void DeleteObject(std::shared_ptr<SceneObject> object) override {
			if(onDelete_) onDelete_(std::move(object));
		}

		void CreateObject(std::shared_ptr<SceneObject> object) override {
			if(onCreate_) onCreate_(std::move(object));
		}

		void RenameObject(std::shared_ptr<SceneObject> object, const std::string& newName) override {
			if(onRename_) onRename_(std::move(object), newName);
		}

		void ApplyPrefabOverrides(std::shared_ptr<SceneObject> object) override {
			if(onApplyPrefab_) onApplyPrefab_(std::move(object));
		}

		void FocusObject(std::shared_ptr<SceneObject> object) override {
			if(onFocus_) onFocus_(std::move(object));
		}

	private:
		SelectCB onSelect_;
		ObjectCB onDelete_;
		ObjectCB onCreate_;
		RenameCB onRename_;
		ObjectCB onApplyPrefab_;
		ObjectCB onFocus_;
	};

} // namespace CalyxEngine
