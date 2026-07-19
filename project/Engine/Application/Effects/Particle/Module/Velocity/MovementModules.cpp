#include "MovementModules.h"

#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

#include <algorithm>
#include <cmath>

namespace CalyxEngine {

	void AccelerationModule::OnUpdate(FxUnit& particle, float deltaTime) {
		particle.velocity += acceleration_ * deltaTime;
	}

	void AccelerationModule::ShowGuiContent() { GuiCmd::DragFloat3("Acceleration", acceleration_); }

	void DragModule::OnUpdate(FxUnit& particle, float deltaTime) {
		particle.velocity *= std::exp(-drag_ * (std::max)(deltaTime, 0.0f));
	}

	void DragModule::ShowGuiContent() {
		if(GuiCmd::DragFloat("Drag", drag_, 0.01f, 0.0f, 1000.0f)) drag_ = (std::max)(drag_, 0.0f);
	}

	void DragModule::SetDrag(float value) { drag_ = (std::max)(value, 0.0f); }

} // namespace CalyxEngine
