#pragma once
/* ========================================================================
/*      include space
/* ===================================================================== */
// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Application/UI/EngineUI/Context/EditorContext.h>

// c++
#include <string>

//forward declaration
class SceneObjectEditor;

class InspectorPanel
	: public IEngineUI {
public:
	InspectorPanel();
	~InspectorPanel() override = default;

	void Render() override;
	const std::string& GetPanelName() const override { return panelName_; }

	void SetSelectedEditor(BaseEditor * editor){ selectedEditor_ = editor; }
	void SetSelectedObject(const std::shared_ptr<SceneObject>&obj);
	void SetSceneObjectEditor(SceneObjectEditor* editor) { sceneObjectEditor_ = editor; }

private:
	BaseEditor * selectedEditor_ = nullptr;
	std::shared_ptr<SceneObject> selectedObject_;
	SceneObjectEditor * sceneObjectEditor_ = nullptr;
};