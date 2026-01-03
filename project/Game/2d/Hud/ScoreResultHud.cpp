#include "ScoreResultHud.h"

#include "HudMotionBuilder.h"
#include "HudMotionGuiHelper.h"
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

///////////////////////////////////////////////////////////////////////////////
//		コンストラクタ / デストラクタ
///////////////////////////////////////////////////////////////////////////////
ScoreResultHud::ScoreResultHud() = default;
ScoreResultHud::~ScoreResultHud() = default;

///////////////////////////////////////////////////////////////////////////////
//		初期化
///////////////////////////////////////////////////////////////////////////////
void ScoreResultHud::Initialize() {
	// Load
	configData_ = std::make_unique<ScoreResultConfig>();
	configData_->LoadParams();

	// BaseHud 側のランタイムモーション構築
	CreateMotionFromConfig();

	// NumbersSprite
	numbers_ = std::make_unique<NumbersSprite>("Textures/Numbers", ".png");
	// stay位置を使いたい場合：posEnd が“滞在”と解釈できる
	numbers_->Initialize(configData_->posEnd, {64, 64});
	numbers_->SetAlign(NumbersSprite::DigitsAlign::Center);
	numbers_->SetValue(0);

	// score timers
	delayTimer_.target_ = configData_->delaySec;
	countTimer_.target_ = configData_->countUpSec;
	countTimer_.easingType_ = static_cast<CalyxEase::EaseType>(configData_->countEaseInt);

	BaseHud::Initialize(static_cast<uint32_t>(Calyx2D::HudMotionChannel::Position));
	Sprite().SetVisibility(false);
}

///////////////////////////////////////////////////////////////////////////////
//		更新
///////////////////////////////////////////////////////////////////////////////
void ScoreResultHud::Update(float dt) {
	BaseHud::Update(dt);
	numbers_->SetPosition(Sprite().GetPosition());

	numbers_->Update();
}

///////////////////////////////////////////////////////////////////////////////
//		描画
///////////////////////////////////////////////////////////////////////////////
void ScoreResultHud::Draw(SpriteRenderer* renderer) const {
	numbers_->Draw(renderer);
}

///////////////////////////////////////////////////////////////////////////////
//		表示スコア設定
////////////////////////////////////////////////////////////////////////////////
void ScoreResultHud::SetScore(uint32_t score) {
	finalScore_ = score;
}

///////////////////////////////////////////////////////////////////////////////
//		滞在フェーズ更新
///////////////////////////////////////////////////////////////////////////////
void ScoreResultHud::StayUpdate(float dt) {
	if(finished_) return;

	// 遅延待ち
	if(!started_) {
		delayTimer_.Update(dt);
		if(delayTimer_.IsReached()) {
			started_ = true;
			countTimer_.Reset();
		}
		return;
	}

	// カウントアップ
	countTimer_.Update(dt);

	float t = std::clamp(countTimer_.t_, 0.0f, 1.0f);
	currentScore_ = finalScore_ * t;

	numbers_->SetValue(static_cast<uint32_t>(currentScore_));

	// 完了
	if(countTimer_.IsReached()) {
		numbers_->SetValue(finalScore_);
		finished_ = true;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
//		GUI表示
///////////////////////////////////////////////////////////////////////////////////////////////
void ScoreResultHud::TopGui() {
	ImGui::Text("SpritePos: %.1f, %.1f", Sprite().GetPosition().x, Sprite().GetPosition().y);
	ImGui::SeparatorText("ScoreResultHud");
	configData_->SaveAndLoadButtonGui();
}

void ScoreResultHud::DerivedGui() {
	bool changed = false;

	// Score 固有
	changed |= GuiCmd::DragFloat("delaySec",   configData_->delaySec);
	changed |= GuiCmd::DragFloat("countUpSec", configData_->countUpSec);
	changed |= CalyxEase::SelectEaseInt("countEase", configData_->countEaseInt);

	// モーション共通GUI
	changed |= Calyx2D::DrawTransformMotionGui(*configData_);

	if (changed) {
		RebuildMotionFromConfig();
		StartEnter();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
//		データからコンフィグ作成
///////////////////////////////////////////////////////////////////////////////////////////////
void ScoreResultHud::CreateMotionFromConfig() {
	// Enter
	Calyx2D::BuildMotionSetFromFlatConfig(*configData_, config_.enterMotion);

	// Exit（start/end 反転）
	Calyx2D::BuildExitMotionSetFromFlatConfig(*configData_, config_.exitMotion);
}

void ScoreResultHud::RebuildMotionFromConfig() {
	// Enter
	Calyx2D::BuildMotionSetFromFlatConfig(*configData_, config_.enterMotion);

	// Exit（start/end 反転）
	Calyx2D::BuildExitMotionSetFromFlatConfig(*configData_, config_.exitMotion);
}