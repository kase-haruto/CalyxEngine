#include "Material.h"

//data
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

#include <externals/imgui/imgui.h>

void Material::ApplyConfig(const MaterialConfig& config){
	color = config.color;
	lightingMode = config.enableLighting;
	shininess = config.shininess;
	enviromentCoefficient = config.enviromentCoefficient;
	isReflect = config.isReflect;
	
}

MaterialConfig Material::ExtractConfig() const{
	MaterialConfig config;
	config.color = color;
	config.enableLighting = lightingMode;
	config.shininess = shininess;
	config.enviromentCoefficient = enviromentCoefficient;
	config.isReflect = isReflect;
	return config;
}

void Material::ShowImGui(){
	static int currentLightingMode_ = 0;
	// lighting
	ImGui::SeparatorText("Lighting");
	GuiCmd::DragFloat("shininess", shininess, 0.01f);
	currentLightingMode_ = std::clamp(lightingMode, 0, 4);

	static const char* lightingModes[] = {
		"Half-Lambert",
		"Lambert",
		"SpecularReflection",
		"No Lighting (Black)",
		"Unlit Color"
	};

	if (ImGui::BeginCombo("Lighting Mode", lightingModes[currentLightingMode_])) {
		for (int n = 0; n < IM_ARRAYSIZE(lightingModes); ++n) {
			bool is_selected = (currentLightingMode_ == n);
			if (ImGui::Selectable(lightingModes[n], is_selected)) {
				currentLightingMode_ = n;
				lightingMode = n;
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	// color
	ImGui::SeparatorText("Color");
	GuiCmd::ColorEdit4("color", color);

	ImGui::SeparatorText("EnviromentCoefficient");
	//環境マップ
	GuiCmd::CheckBox("isReflect", isReflect);
	if (isReflect){
		GuiCmd::SliderFloat("enviromentCoefficient", enviromentCoefficient, 0.0f, 1.0f);
		GuiCmd::SliderFloat("roughness", roughness, 0.0f, 1.0f);
	}
}

void Material::ShowImGui(MaterialConfig& config){
	static int currentLightingMode_ = 0;
	// lighting
	ImGui::SeparatorText("Lighting");
	GuiCmd::DragFloat("shininess", config.shininess, 0.01f);
	const char* lightingModes[] = {"Half-Lambert", "Lambert", "SpecularReflection", "No Lighting"};
	if (ImGui::BeginCombo("Lighting Mode", lightingModes[currentLightingMode_])){
		for (int n = 0; n < IM_ARRAYSIZE(lightingModes); n++){
			bool is_selected = (currentLightingMode_ == n);
			if (ImGui::Selectable(lightingModes[n], is_selected)){
				currentLightingMode_ = n;
				config.enableLighting = currentLightingMode_;
			}
			if (is_selected){
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	// color
	ImGui::SeparatorText("Color");
	GuiCmd::ColorEdit4("color", config.color);

	ImGui::SeparatorText("EnviromentCoefficient");
	//環境マップ
	GuiCmd::CheckBox("isReflect", config.isReflect);
	if (config.isReflect){
		GuiCmd::SliderFloat("enviromentCoefficient", config.enviromentCoefficient, 0.0f, 1.0f);
		GuiCmd::SliderFloat("roughness", roughness, 0.0f, 1.0f);
	}
	ApplyConfig(config);
}

