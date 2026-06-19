#include "PhysicsBody.h"

#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <externals/imgui/imgui.h>

#include <algorithm>

void PhysicsBody::ShowGui() {
	if(ImGui::TreeNodeEx("PhysicsBody", ImGuiTreeNodeFlags_SpanAvailWidth)) {
		GuiCmd::CheckBox("Enable Physics Response", enabled_);

		int bodyType = static_cast<int>(bodyType_);
		const char* items[] = {"Static", "Kinematic"};
		if(GuiCmd::Combo("Body Type", bodyType, items, 2)) {
			bodyType_ = static_cast<PhysicsBodyType>(bodyType);
		}

		GuiCmd::DragFloat("Pushback Ratio", pushbackRatio_, 0.01f, 0.0f, 1.0f);
		pushbackRatio_ = std::clamp(pushbackRatio_, 0.0f, 1.0f);

		ImGui::TreePop();
	}
}

void PhysicsBody::ApplyConfig(const PhysicsBodyConfig& config) {
	enabled_ = config.enabled;
	bodyType_ = static_cast<PhysicsBodyType>(std::clamp(config.bodyType, 0, 1));
	pushbackRatio_ = std::clamp(config.pushbackRatio, 0.0f, 1.0f);
}

PhysicsBodyConfig PhysicsBody::ExtractConfig() const {
	PhysicsBodyConfig config;
	config.enabled = enabled_;
	config.bodyType = static_cast<int>(bodyType_);
	config.pushbackRatio = pushbackRatio_;
	return config;
}
