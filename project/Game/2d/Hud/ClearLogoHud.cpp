#include "ClearLogoHud.h"

#include "Data/Game/Config/Hud/ClearHudConfig.h"
#include "Engine/Application/System/Environment.h"
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"
#include "HudMotionBuilder.h"
#include "HudMotionGuiHelper.h"

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

	Sprite().SetScale(configData_->logoSize);

	floatingAnimation_ = std::make_unique<CalyxUtil::SimpleAnimation<float>>();
	floatingAnimation_->SetLoopCount(0); // 無限ループ


	floatingAnimation_->SetStart(-configData_->amplitude);
	floatingAnimation_->SetEnd(configData_->amplitude);
	floatingAnimation_->SetDuration(configData_->period);
	// PingPong にして上下往復させる
	floatingAnimation_->SetLoopType(CalyxUtil::AnimationLoop::AnimationLoopType::PingPong);
	floatingAnimation_->Start();

	// 基準位置はまだ取得していない
	floatingBaseSet_ = false;
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

void ClearLogoHud::StayUpdate(float dt) {
	if(!floatingAnimation_) {
		return;
	}

	// 基準位置を一度だけ取得（モーション再構築時にリセットされる）
	if(!floatingBaseSet_) {
		auto pos = Sprite().GetPosition();
		floatingBaseX_ = pos.x;
		floatingBaseY_ = pos.y;
		floatingBaseSet_ = true;
	}

	// アニメーション更新してオフセット取得
	float offset = 0.0f;
	floatingAnimation_->LerpValue(offset, dt);

	// 基準位置にオフセットを合成してスプライト位置を更新
	Sprite().SetPosition({ floatingBaseX_, floatingBaseY_ + offset });
}

///////////////////////////////////////////////////////////////////////////////////////////////
//		データからコンフィグ作成
///////////////////////////////////////////////////////////////////////////////////////////////
void ClearLogoHud::RebuildMotionFromConfig() {
	// テクスチャ（固定）
	config_.texturePath = "Textures/ResultHud/resultLogo.png";

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