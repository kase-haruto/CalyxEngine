#include "SizeOverLiftimeModule.h"
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>

// c++

namespace CalyxEngine {
	SizeOverLiftimeModule::SizeOverLiftimeModule(const std::string name)
		: CalyxEngine::BaseFxModule(name) {}

	/////////////////////////////////////////////////////////////////////////////////////////
	// 適用
	/////////////////////////////////////////////////////////////////////////////////////////
	void SizeOverLiftimeModule::OnUpdate(FxUnit& unit, [[maybe_unused]] float dt) {
		if(unit.lifetime <= 0.0f) return; // ゼロ割り防止

		float t = unit.age / unit.lifetime;
		if(t > 1.0f) t = 1.0f;

		float easeT		 = CalyxEngine::ApplyEase(easeType_, t);
		float sizeFactor = isGrowing_ ? easeT : (1.0f - easeT);

		unit.scale = unit.initialScale * sizeFactor;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	// gui表示
	/////////////////////////////////////////////////////////////////////////////////////////
	void SizeOverLiftimeModule::ShowGuiContent() {
		GuiCmd::CheckBox("isGrowing", isGrowing_);

		CalyxEngine::SelectEase(easeType_);
	}
}
