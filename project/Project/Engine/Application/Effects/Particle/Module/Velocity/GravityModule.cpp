#include "GravityModule.h"
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

// c++

GravityModule::GravityModule(const std::string name):BaseFxModule(name){}

/////////////////////////////////////////////////////////////////////////////////////////
//	重力適用
/////////////////////////////////////////////////////////////////////////////////////////
void GravityModule::OnUpdate(FxUnit& unit, float dt){
	unit.velocity += gravity_ * dt; // 重力を速度に加算
}

/////////////////////////////////////////////////////////////////////////////////////////
//	gui表示
/////////////////////////////////////////////////////////////////////////////////////////
void GravityModule::ShowGuiContent(){
	GuiCmd::DragFloat3("strength", gravity_);
}
