#include "EditorContext.h"

void EditorContext::SetSelectedObject(SceneObject* obj) {
	selectedObject_ = obj;
}

SceneObject* EditorContext::GetSelectedObject() const {
	return selectedObject_;
}

void EditorContext::SetSelectedEditor(BaseEditor* editor) {
	selectedEditor_ = editor;
}

BaseEditor* EditorContext::GetSelectedEditor() const {
	return selectedEditor_;
}
