#include "ClearLogoHud.h"

#include "HudMotionBuilder.h"
#include "HudMotionGuiHelper.h"
#include "Data/Game/Config/Hud/ClearHudConfig.h"
#include "Engine/Application/System/Enviroment.h"
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

ClearLogoHud::ClearLogoHud()  = default;
ClearLogoHud::~ClearLogoHud() = default;

///////////////////////////////////////////////////////////////////////////////////////////////
//		初期化
///////////////////////////////////////////////////////////////////////////////////////////////
void ClearLogoHud::Initialize() {
	// Config 読み込み
	configData_ = std::make_unique<ClearLogoHudConfig>();
	configData_->LoadParams();

	// Config → Runtime
	RebuildMotionFromConfig();

	// BaseHud 初期化（Position を使用）
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
	if(Calyx2D::DrawTransformMotionGui(*configData_)) {
		RebuildMotionFromConfig();
		StartEnter();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
//		データからコンフィグ作成
///////////////////////////////////////////////////////////////////////////////////////////////
void ClearLogoHud::RebuildMotionFromConfig() {
	// テクスチャ（固定）
	config_.texturePath = "Textures/uvChecker.png";

	// Enter Motion
	Calyx2D::BuildMotionSetFromFlatConfig(
		*configData_,
		config_.enterMotion
	);

	// Exit Motion（start/end 反転）
	Calyx2D::BuildExitMotionSetFromFlatConfig(
		*configData_,
		config_.exitMotion
	);
}