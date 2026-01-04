#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Application/UI/EngineUI/Context/EditorContext.h>
// c++
#include <unordered_map>
#include <memory>
#include <string>

class PanelController {
public:
	//===================================================================*/
	//                   public functions
	//===================================================================*/
	PanelController() = default;
	~PanelController() = default;

	void Initialize();
	void RenderPanels();
	void RegisterPanel(const std::string name,std::unique_ptr<IEngineUI> panel);
	IEngineUI* GetPanel(const std::string& name);

private:
	//===================================================================*/
	//                   private variables
	//===================================================================*/
	std::unordered_map<std::string, std::unique_ptr<IEngineUI>> panels_;
	std::unique_ptr<EditorContext> editorContext_;
};

