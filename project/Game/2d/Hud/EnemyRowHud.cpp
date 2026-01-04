#include "EnemyRowHud.h"

#include "HudMotionBuilder.h"
#include "HudMotionGuiHelper.h"
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

EnemyResultHud::EnemyResultHud()  = default;
EnemyResultHud::~EnemyResultHud() = default;

///////////////////////////////////////////////////////////////////////////////
//		初期化
///////////////////////////////////////////////////////////////////////////////
void EnemyResultHud::Initialize(const ResultTransitionPayload& payload) {
	// Load
	configData_ = std::make_unique<EnemyResultConfig>();
	configData_->LoadParams();

	// BaseHud 側のランタイムモーション構築
	CreateMotionFromConfig();

	// 行生成
	rows_.clear();
	rows_.reserve(payload.results.size());

	// アンカーは config の posEnd（ScoreResultHudと同じ解釈に合わせる）
	// BaseHud の Sprite は非表示のアンカー扱い
	// NumbersSprite 初期化位置はここでは仮。Updateで毎フレ row に追従させる。
	float  y     = 0.0f;
	size_t index = 0;

	for(const auto& e : payload.results) {
		EnemyRow row;

		row.finalCount   = static_cast<uint32_t>(e.count);
		row.currentCount = 0.0f;

		row.rowY = y;

		// icon
		row.icon = std::make_unique<Calyx2D::SpriteObject2d>();
		if(e.kind == EnemyKind::Boss) {
			row.icon->Initialize("Textures/ResultHud/bossDead.png");
		}else {
			row.icon->Initialize("Textures/ResultHud/enemyDead.png");
		}
		row.icon->SetScale(configData_->iconSize);
		row.icon->SetPosition(configData_->posEnd + CalyxMath::Vector2{0.0f,row.rowY} + configData_->iconOffset);

		// numbers
		row.numbers = std::make_unique<NumbersSprite>("Textures/Numbers",".png");
		row.numbers->Initialize(configData_->posEnd + CalyxMath::Vector2{0.0f,row.rowY} + configData_->numberOffset,
								configData_->digitSize);
		row.numbers->SetAlign(NumbersSprite::DigitsAlign::Center);
		row.numbers->SetValue(0);

		// timers（行ごとに追加ディレイ）
		row.delayTimer.target_     = configData_->delaySec + (static_cast<float>(index) * configData_->perRowDelaySec);
		row.countTimer.target_     = configData_->countUpSec;
		row.countTimer.easingType_ = static_cast<CalyxEase::EaseType>(configData_->countEaseInt);

		rows_.push_back(std::move(row));

		y += configData_->rowSpacing;
		++index;
	}

	// BaseHud 初期化：アンカーのPositionだけ使う（ScoreResultHudと同じ）
	BaseHud::Initialize(static_cast<uint32_t>(Calyx2D::HudMotionChannel::Position));

	// アンカースプライトは非表示（ScoreResultHudと同じ）
	Sprite().SetVisibility(false);
}

///////////////////////////////////////////////////////////////////////////////
//		更新
///////////////////////////////////////////////////////////////////////////////
void EnemyResultHud::Update(float dt) {
	BaseHud::Update(dt);

	// アンカー位置に追従してレイアウト更新
	const CalyxMath::Vector2 anchor = Sprite().GetPosition();

	for(auto& row : rows_) {
		const CalyxMath::Vector2 rowBase = anchor + CalyxMath::Vector2{0.0f,row.rowY};

		row.icon->SetPosition(rowBase + configData_->iconOffset);
		row.numbers->SetPosition(rowBase + configData_->numberOffset);

		row.icon->Update(dt);
		row.numbers->Update();
	}
}

///////////////////////////////////////////////////////////////////////////////
//		描画
///////////////////////////////////////////////////////////////////////////////
void EnemyResultHud::Draw(SpriteRenderer* renderer) const {
	for(const auto& row : rows_) {
		row.icon->Draw(renderer);
		row.numbers->Draw(renderer);
	}
}

///////////////////////////////////////////////////////////////////////////////
//		滞在フェーズ更新
///////////////////////////////////////////////////////////////////////////////
void EnemyResultHud::StayUpdate(float dt) {
	for(auto& row : rows_) {

		// 遅延待ち
		if(!row.delayTimer.IsReached()) {
			row.delayTimer.Update(dt);
			continue;
		}

		// カウントアップ
		row.countTimer.Update(dt);

		float t          = std::clamp(row.countTimer.t_,0.0f,1.0f);
		row.currentCount = static_cast<float>(row.finalCount) * t;

		row.numbers->SetValue(static_cast<uint32_t>(row.currentCount));

		// 完了
		if(row.countTimer.IsReached()) { row.numbers->SetValue(row.finalCount); }
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
//		GUI表示
///////////////////////////////////////////////////////////////////////////////////////////////
void EnemyResultHud::TopGui() {
	ImGui::Text("Anchor(Sprite) Pos: %.1f, %.1f", Sprite().GetPosition().x, Sprite().GetPosition().y);
	ImGui::SeparatorText("EnemyResultHud");
	configData_->SaveAndLoadButtonGui();
}

void EnemyResultHud::DerivedGui() {
	bool changed = false;

	// EnemyResult 固有
	changed |= GuiCmd::DragFloat("delaySec",        configData_->delaySec);
	changed |= GuiCmd::DragFloat("perRowDelaySec",  configData_->perRowDelaySec);
	changed |= GuiCmd::DragFloat("countUpSec",      configData_->countUpSec);
	changed |= GuiCmd::DragFloat("rowSpacing",      configData_->rowSpacing);
	changed |= CalyxEase::SelectEaseInt("countEase", configData_->countEaseInt);

	// offset/size
	changed |= GuiCmd::DragFloat2("iconOffset",   configData_->iconOffset);
	changed |= GuiCmd::DragFloat2("numberOffset", configData_->numberOffset);
	changed |= GuiCmd::DragFloat2("iconSize",     configData_->iconSize);
	changed |= GuiCmd::DragFloat2("digitSize",    configData_->digitSize);

	// モーション共通GUI（ScoreResultHudと同じ）
	changed |= Calyx2D::DrawTransformMotionGui(*configData_);

	if(changed) {
		RebuildMotionFromConfig();
		StartEnter();

		// ついでにタイマー再設定（GUI変更が反映されやすい）
		size_t idx = 0;
		for(auto& row : rows_) {
			row.delayTimer.target_ = configData_->delaySec + (static_cast<float>(idx) * configData_->perRowDelaySec);
			row.delayTimer.Reset();
			row.countTimer.target_ = configData_->countUpSec;
			row.countTimer.Reset();
			row.countTimer.easingType_ = static_cast<CalyxEase::EaseType>(configData_->countEaseInt);

			row.currentCount = 0.0f;
			row.numbers->SetValue(0);

			++idx;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
//		データからモーション構築（ScoreResultHudと同じ）
///////////////////////////////////////////////////////////////////////////////////////////////
void EnemyResultHud::CreateMotionFromConfig() {
	// Enter
	Calyx2D::BuildMotionSetFromFlatConfig(*configData_, config_.enterMotion);

	// Exit（start/end 反転）
	Calyx2D::BuildExitMotionSetFromFlatConfig(*configData_, config_.exitMotion);
}

void EnemyResultHud::RebuildMotionFromConfig() {
	// Enter
	Calyx2D::BuildMotionSetFromFlatConfig(*configData_, config_.enterMotion);

	// Exit（start/end 反転）
	Calyx2D::BuildExitMotionSetFromFlatConfig(*configData_, config_.exitMotion);
}