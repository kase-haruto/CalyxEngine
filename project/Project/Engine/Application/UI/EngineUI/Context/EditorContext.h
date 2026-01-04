#pragma once
/* ========================================================================
/* include
/* ===================================================================== */
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Editor/BaseEditor.h>

class EditorContext {
public:
	//===================================================================*/
	//		public function
	//===================================================================*/
	EditorContext() = default;
	~EditorContext() = default;

	//--------- accessor -----------------------------------------------------
	void SetSelectedObject(SceneObject* obj);
	SceneObject* GetSelectedObject() const;
	void SetSelectedEditor(BaseEditor* editor);
	BaseEditor* GetSelectedEditor() const;


private:
	//===================================================================*/
	//		private variable
	//===================================================================*/
	SceneObject* selectedObject_ = nullptr;	// 選択された SceneObject
	BaseEditor* selectedEditor_ = nullptr;	// 選択された Editor
};

