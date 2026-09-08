#include "Material.h"

//data
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

#include <externals/imgui/imgui.h>

#include <algorithm>

void Material::ApplyConfig(const MaterialConfig& config) {
	color                 = config.color;
	lightingMode          = config.enableLighting;
	shininess             = config.shininess;
	envirometCoefficient = config.enviromentCoefficient;
	isReflect             = config.isReflect ? 1 : 0;
	roughness             = config.roughness;
    toonHighlightColor    = config.toonHighlightColor;
    toonBaseColor         = config.toonBaseColor;
    toonMidShadowColor    = config.toonMidShadowColor;
    toonShadowColor       = config.toonShadowColor;
    toonBaseStep          = config.toonBaseStep;
    toonBaseFeather       = config.toonBaseFeather;
    toonShadeStep         = config.toonShadeStep;
    toonShadeFeather      = config.toonShadeFeather;
    toonSpecularThreshold = config.toonSpecularThreshold;
    toonSpecularSoftness  = config.toonSpecularSoftness;
    toonSpecularIntensity = config.toonSpecularIntensity;
    emissiveColor         = config.emissiveColor;
    emissiveIntensity     = config.emissiveIntensity;
	useNormalMap          = config.useNormalMap ? 1 : 0;
	normalMapStrength     = config.normalMapStrength;
	normalMapFlipY        = config.normalMapFlipY ? 1 : 0;
	rimColor              = config.rimColor;
	rimIntensity          = config.rimIntensity;
	rimPower              = config.rimPower;
	shieldColor           = config.shieldColor;
	shieldParams          = {config.shieldEnabled ? 1.0f : 0.0f, config.shieldFresnelPower, config.shieldEmission, config.shieldNoiseScale};
	shieldRipple.x        = config.shieldRippleSpeed;
	shieldRipple.y        = config.shieldRippleWidth;

}

MaterialConfig Material::ExtractConfig() const {
	MaterialConfig config;
	config.color                 = color;
	config.enableLighting        = lightingMode;
	config.shininess             = shininess;
	config.enviromentCoefficient = envirometCoefficient;
	config.isReflect             = isReflect != 0;
	config.roughness             = roughness;
    config.toonHighlightColor    = toonHighlightColor;
    config.toonBaseColor         = toonBaseColor;
    config.toonMidShadowColor    = toonMidShadowColor;
    config.toonShadowColor       = toonShadowColor;
    config.toonBaseStep          = toonBaseStep;
    config.toonBaseFeather       = toonBaseFeather;
    config.toonShadeStep         = toonShadeStep;
    config.toonShadeFeather      = toonShadeFeather;
    config.toonThreshold1        = toonShadeStep;
    config.toonThreshold2        = toonBaseStep;
    config.toonThreshold3        = 0.82f;
    config.toonEdgeSoftness      = std::max(toonShadeFeather, toonBaseFeather);
    config.toonSpecularThreshold = toonSpecularThreshold;
    config.toonSpecularSoftness  = toonSpecularSoftness;
    config.toonSpecularIntensity = toonSpecularIntensity;
    config.emissiveColor         = emissiveColor;
    config.emissiveIntensity     = emissiveIntensity;
	config.useNormalMap          = useNormalMap != 0;
	config.normalMapStrength     = normalMapStrength;
	config.normalMapFlipY        = normalMapFlipY != 0;
	config.rimColor              = rimColor;
	config.rimIntensity          = rimIntensity;
	config.rimPower              = rimPower;
	config.shieldEnabled         = shieldParams.x > 0.5f;
	config.shieldColor           = shieldColor;
	config.shieldFresnelPower    = shieldParams.y;
	config.shieldEmission        = shieldParams.z;
	config.shieldNoiseScale      = shieldParams.w;
	config.shieldRippleSpeed     = shieldRipple.x;
	config.shieldRippleWidth     = shieldRipple.y;
	return config;
}

void Material::ShowImGui() {
    static int currentLightingMode_ = 0;

    // lighting
    ImGui::SeparatorText("Lighting");
    GuiCmd::DragFloat("shininess", shininess, 0.01f);

    static constexpr const char* lightingModes[] = {
        "Half-Lambert",
        "Lambert",
        "Toon",
        "No Lighting (Black)",
        "Unlit Color"
    };
    constexpr int lightingModeCount = static_cast<int>(std::size(lightingModes));

    currentLightingMode_ = std::clamp(lightingMode, 0, lightingModeCount - 1);
    const char* currentModeLabel = lightingModes[std::clamp(currentLightingMode_, 0, lightingModeCount - 1)];

    if (ImGui::BeginCombo("Lighting Mode", currentModeLabel)) {
        for (int n = 0; n < lightingModeCount; ++n) {
            bool is_selected = (currentLightingMode_ == n);
            if (ImGui::Selectable(lightingModes[n], is_selected)) {
                currentLightingMode_ = n;
                lightingMode = n;
            }
            if (is_selected) { ImGui::SetItemDefaultFocus(); }
        }
        ImGui::EndCombo();
    }

    // color
    ImGui::SeparatorText("Color");
    GuiCmd::ColorEdit4("color", color);

    ImGui::SeparatorText("Emissive");
    GuiCmd::ColorEdit4("emissive color", emissiveColor);
    GuiCmd::SliderFloat("emissive intensity", emissiveIntensity, 0.0f, 20.0f);
	ImGui::SeparatorText("Rim Light");
	GuiCmd::ColorEdit4("rim color", rimColor);
	GuiCmd::SliderFloat("rim intensity", rimIntensity, 0.0f, 10.0f);
	GuiCmd::SliderFloat("rim power", rimPower, 0.1f, 16.0f);
	ImGui::SeparatorText("Shield");
	bool shieldEnabled = shieldParams.x > 0.5f;
	if(GuiCmd::CheckBox("shield enabled", shieldEnabled)) shieldParams.x = shieldEnabled ? 1.0f : 0.0f;
	GuiCmd::ColorEdit4("shield color", shieldColor);
	GuiCmd::SliderFloat("shield fresnel power", shieldParams.y, 0.25f, 16.0f);
	GuiCmd::SliderFloat("shield emission", shieldParams.z, 0.0f, 12.0f);
	GuiCmd::SliderFloat("shield noise scale", shieldParams.w, 0.1f, 16.0f);
	GuiCmd::SliderFloat("shield ripple speed", shieldRipple.x, 0.0f, 8.0f);
	GuiCmd::SliderFloat("shield ripple width", shieldRipple.y, 0.01f, 0.5f);

	ImGui::SeparatorText("Normal Map");
	bool normalMapEnabled = useNormalMap != 0;
	if(GuiCmd::CheckBox("use normal map", normalMapEnabled)) {
		useNormalMap = normalMapEnabled ? 1 : 0;
	}
	GuiCmd::SliderFloat("normal map strength", normalMapStrength, 0.0f, 2.0f);
	bool flipY = normalMapFlipY != 0;
	if(GuiCmd::CheckBox("flip normal map Y", flipY)) {
		normalMapFlipY = flipY ? 1 : 0;
	}

    if (lightingMode == 2) {
        ImGui::SeparatorText("Toon");
        GuiCmd::ColorEdit4("highlight", toonHighlightColor);
        GuiCmd::ColorEdit4("base ramp", toonBaseColor);
        GuiCmd::ColorEdit4("mid shadow", toonMidShadowColor);
        GuiCmd::ColorEdit4("shadow", toonShadowColor);
        GuiCmd::SliderFloat("base step", toonBaseStep, -1.0f, 1.0f);
        GuiCmd::SliderFloat("base feather", toonBaseFeather, 0.0f, 0.25f);
        GuiCmd::SliderFloat("shade step", toonShadeStep, -1.0f, 1.0f);
        GuiCmd::SliderFloat("shade feather", toonShadeFeather, 0.0f, 0.25f);
        GuiCmd::SliderFloat("specular threshold", toonSpecularThreshold, 0.0f, 1.0f);
        GuiCmd::SliderFloat("specular softness", toonSpecularSoftness, 0.0f, 0.25f);
        GuiCmd::SliderFloat("specular intensity", toonSpecularIntensity, 0.0f, 4.0f);
    }

    ImGui::SeparatorText("EnviromentCoefficient");
    // 環境マップ
    bool reflect = isReflect != 0;
    if(GuiCmd::CheckBox("isReflect", reflect)) {
        isReflect = reflect ? 1 : 0;
    }
    if (isReflect != 0) {
        GuiCmd::SliderFloat("enviromentCoefficient", envirometCoefficient, 0.0f, 1.0f);
        GuiCmd::SliderFloat("roughness", roughness, 0.0f, 1.0f);
    }
}

void Material::ShowImGui(MaterialConfig& config) {
    static int currentLightingMode_ = 0;

    // color
    if (ImGui::TreeNodeEx("Color", ImGuiTreeNodeFlags_SpanAvailWidth)) {
        GuiCmd::ColorEdit4("color", config.color);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Emissive", ImGuiTreeNodeFlags_SpanAvailWidth)) {
        GuiCmd::ColorEdit4("emissive color", config.emissiveColor);
        GuiCmd::SliderFloat("emissive intensity", config.emissiveIntensity, 0.0f, 20.0f);
        ImGui::TreePop();
    }
	if(ImGui::TreeNodeEx("Rim Light", ImGuiTreeNodeFlags_SpanAvailWidth)) {
		GuiCmd::ColorEdit4("rim color", config.rimColor);
		GuiCmd::SliderFloat("rim intensity", config.rimIntensity, 0.0f, 10.0f);
		GuiCmd::SliderFloat("rim power", config.rimPower, 0.1f, 16.0f);
		ImGui::TreePop();
	}
	if(ImGui::TreeNodeEx("Shield", ImGuiTreeNodeFlags_SpanAvailWidth)) {
		GuiCmd::CheckBox("enabled", config.shieldEnabled);
		GuiCmd::ColorEdit4("color", config.shieldColor);
		GuiCmd::SliderFloat("fresnel power", config.shieldFresnelPower, 0.25f, 16.0f);
		GuiCmd::SliderFloat("emission", config.shieldEmission, 0.0f, 12.0f);
		GuiCmd::SliderFloat("noise scale", config.shieldNoiseScale, 0.1f, 16.0f);
		GuiCmd::SliderFloat("ripple speed", config.shieldRippleSpeed, 0.0f, 8.0f);
		GuiCmd::SliderFloat("ripple width", config.shieldRippleWidth, 0.01f, 0.5f);
		ImGui::TreePop();
	}

	if(ImGui::TreeNodeEx("Normal Map", ImGuiTreeNodeFlags_SpanAvailWidth)) {
		GuiCmd::CheckBox("use normal map", config.useNormalMap);
		GuiCmd::SliderFloat("normal map strength", config.normalMapStrength, 0.0f, 2.0f);
		GuiCmd::CheckBox("flip normal map Y", config.normalMapFlipY);
		ImGui::TreePop();
	}

    if (config.enableLighting == 2 && ImGui::TreeNodeEx("Toon", ImGuiTreeNodeFlags_SpanAvailWidth)) {
        GuiCmd::ColorEdit4("highlight", config.toonHighlightColor);
        GuiCmd::ColorEdit4("base ramp", config.toonBaseColor);
        GuiCmd::ColorEdit4("mid shadow", config.toonMidShadowColor);
        GuiCmd::ColorEdit4("shadow", config.toonShadowColor);
        GuiCmd::SliderFloat("base step", config.toonBaseStep, -1.0f, 1.0f);
        GuiCmd::SliderFloat("base feather", config.toonBaseFeather, 0.0f, 0.25f);
        GuiCmd::SliderFloat("shade step", config.toonShadeStep, -1.0f, 1.0f);
        GuiCmd::SliderFloat("shade feather", config.toonShadeFeather, 0.0f, 0.25f);
        GuiCmd::SliderFloat("specular threshold", config.toonSpecularThreshold, 0.0f, 1.0f);
        GuiCmd::SliderFloat("specular softness", config.toonSpecularSoftness, 0.0f, 0.25f);
        GuiCmd::SliderFloat("specular intensity", config.toonSpecularIntensity, 0.0f, 4.0f);
        ImGui::TreePop();
    }

    // lighting
    if (ImGui::TreeNodeEx("Lighting", ImGuiTreeNodeFlags_SpanAvailWidth)) {
        GuiCmd::DragFloat("shininess", config.shininess, 0.01f);

        static constexpr const char* lightingModes[] = {
            "Half-Lambert",
            "Lambert",
            "Toon",
            "No Lighting (Black)",
            "Unlit Color"
        };
        constexpr int lightingModeCount = static_cast<int>(std::size(lightingModes));

        currentLightingMode_ = std::clamp(config.enableLighting, 0, lightingModeCount - 1);
        const char* currentModeLabel = lightingModes[std::clamp(currentLightingMode_, 0, lightingModeCount - 1)];

        if (ImGui::BeginCombo("Lighting Mode", currentModeLabel)) {
            for (int n = 0; n < lightingModeCount; ++n) {
                bool is_selected = (currentLightingMode_ == n);
                if (ImGui::Selectable(lightingModes[n], is_selected)) {
                    currentLightingMode_ = n;
                    config.enableLighting = currentLightingMode_;
                }
                if (is_selected) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndCombo();
        }

        ImGui::TreePop();
    }

    // 環境マップ
    if (ImGui::TreeNodeEx("EnviromentCoefficient", ImGuiTreeNodeFlags_SpanAvailWidth)) {
        GuiCmd::CheckBox("isReflect", config.isReflect);
        if (config.isReflect) {
            GuiCmd::SliderFloat("enviromentCoefficient", config.enviromentCoefficient, 0.0f, 1.0f);
            GuiCmd::SliderFloat("roughness", config.roughness, 0.0f, 1.0f);
        }
        ImGui::TreePop();
    }

    ApplyConfig(config);
}
