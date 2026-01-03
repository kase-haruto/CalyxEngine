#include "ClearLogoHud.h"

#include "Data/Game/Config/Hud/ClearHudConfig.h"
#include "Engine/Application/System/Enviroment.h"
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

ClearLogoHud::ClearLogoHud()  = default;
ClearLogoHud::~ClearLogoHud() = default;

///////////////////////////////////////////////////////////////////////////////////////////////
//		初期化
///////////////////////////////////////////////////////////////////////////////////////////////
void ClearLogoHud::Initialize() {
	configData_ = std::make_unique<ClearLogoHudConfig>();
	configData_->LoadParams();

	CreateConfigFromData();
	BaseHud::Initialize(static_cast<uint32_t>(Calyx2D::HudMotionChannel::Position));
}

///////////////////////////////////////////////////////////////////////////////////////////////
//		GUI表示
///////////////////////////////////////////////////////////////////////////////////////////////
void ClearLogoHud::TopGui() {
	ImGui::SeparatorText("ClearLogoHud");
	configData_->SaveAndLoadButtonGui();
}

void ClearLogoHud::DerivedGui() {
	GuiCmd::DragFloat2("startPosition", configData_->startPosition);
	GuiCmd::DragFloat2("stayPosition", configData_->stayPosition);
	GuiCmd::DragFloat2("scale", configData_->scale);
	GuiCmd::DragFloat("duration", configData_->duration, 0.1f);

	if(ImGui::IsItemDeactivatedAfterEdit()) {
		CreateConfigFromData();
		StartEnter();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
//		データからコンフィグ作成
///////////////////////////////////////////////////////////////////////////////////////////////
void ClearLogoHud::CreateConfigFromData() {
	config_.texturePath = "Textures/white1x1.png";

	config_.enterMotion.position = Calyx2D::HudMotionDesc<CalyxMath::Vector2>{
		.start = configData_->startPosition,
		.end = configData_->stayPosition,
		.duration = configData_->duration,
		.easing = configData_->easeType
	};

	config_.exitMotion.position = Calyx2D::HudMotionDesc<CalyxMath::Vector2>{
		.start = configData_->stayPosition,
		.end = configData_->startPosition,
		.duration = configData_->duration,
		.easing = configData_->easeType
	};
}