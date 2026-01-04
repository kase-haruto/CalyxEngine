#pragma once
/* ========================================================================
/*      include space
/* ===================================================================== */
// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>

// c++
#include <vector>
#include <string>

class ConsolePanel
    : public IEngineUI{
public:
    //===================================================================*/
    //                   public funclion
    //===================================================================*/
    ConsolePanel();
    ~ConsolePanel() override = default;

    void Render() override;
    const std::string& GetPanelName() const override;

    void AddLog(const std::string& log);

private:
    //===================================================================*/
    //                   private variable
    //===================================================================*/
	int selectedLogType_ = 0;
};

