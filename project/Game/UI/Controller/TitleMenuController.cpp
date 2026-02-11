#include "TitleMenuController.h"

// engine
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Input/Input.h>

// externals
#include <externals/imgui/imgui.h>

// engine
#include <Game/UI/Button/Button.h>

// 指定のEase
#include <Engine/Foundation/Utility/Ease/Ease.h>

TitleMenuController::TitleMenuController()  {
	config_.LoadParams();

	// basePos_ を変更しないようローカル pos を使う
	CalyxMath::Vector2 pos = config_.basePos_;

	// ゲームスタートボタン
	std::unique_ptr<Button> startButton =
		std::make_unique<Button>("Textures/gameStart_titleButton.dds",
								 pos,
								 config_.baseSize_);
	pos.y += config_.space_;

	std::unique_ptr<Button> exitButton =
		std::make_unique<Button>("Textures/endGame_titleButton.dds",
								 pos,
								 config_.baseSize_);

	// リストに追加
	buttons_.push_back(std::move(startButton));
	buttons_.push_back(std::move(exitButton));

	// 初期選択
	selectedIndex_ = 0;
	for (size_t i = 0; i < buttons_.size(); ++i) {
		buttons_[i]->SetSelected(i == selectedIndex_);
	}

	// アニメ係数を初期化（選択中=1, それ以外=0）
	selectedAnimT_.assign(buttons_.size(), 0.0f);
	if (!selectedAnimT_.empty()) selectedAnimT_[selectedIndex_] = 1.0f;

	// 初回レイアウト
	AdaptationForSprite();
}

TitleMenuController::~TitleMenuController() = default;

void TitleMenuController::Update(float dt) {
	// --- 入力で選択移動 ---
	auto* in	   = CalyxFoundation::Input::GetInstance();
	bool moveDown = in->TriggerKey(DIK_DOWN) || in->TriggerKey(DIK_S) || in->TriggerGamepadButton(CalyxFoundation::PadButton::DPAD_DOWN);
	bool moveUp = in->TriggerKey(DIK_UP) || in->TriggerKey(DIK_W)
		|| in->TriggerGamepadButton(CalyxFoundation::PadButton::DPAD_UP);

	if (!buttons_.empty()) {
		uint16_t prev = selectedIndex_;
		if (moveDown) {
			selectedIndex_ = static_cast<uint16_t>((selectedIndex_ + 1) % buttons_.size());
		} else if (moveUp) {
			selectedIndex_ = static_cast<uint16_t>((selectedIndex_ + buttons_.size() - 1) % buttons_.size());
		}
		if (prev != selectedIndex_) {
			for (size_t i = 0; i < buttons_.size(); ++i) {
				buttons_[i]->SetSelected(i == selectedIndex_);
			}
		}

		if (in->TriggerKey(DIK_SPACE) || in->TriggerGamepadButton(CalyxFoundation::PadButton::A)) {
			buttons_[selectedIndex_]->Execute();
		}
	}

	// --- 選択拡大アニメの t を更新 ---
	if (selectedAnimT_.size() != buttons_.size()) {
		selectedAnimT_.assign(buttons_.size(), 0.0f);
	}
	for (size_t i = 0; i < buttons_.size(); ++i) {
		const float target = (i == selectedIndex_) ? 1.0f : 0.0f;
		float t = selectedAnimT_[i];
		const float step = config_.animSpeed_ * dt;
		if (t < target) {
			t = (t + step < target) ? (t + step) : target;
		} else if (t > target) {
			t = (t - step > target) ? (t - step) : target;
		}
		selectedAnimT_[i] = t;
	}

	// --- 拡大を考慮したレイアウト ---
	LayoutButtons_();

	// --- いつもの更新 ---
	for (auto& b : buttons_) {
		b->Update(dt);
	}
}

void TitleMenuController::ShowGui() {
	ImGui::Begin("menuController");
	ImGui::Text("selectedIndex = %d", selectedIndex_);
	ImGui::SeparatorText("baseParams");

	bool changed = false;
	changed |= config_.ShowGui();


	if (changed) {
		AdaptationForSprite(); // GUI変更時に即反映
	}

	ImGui::SeparatorText("sprites");
	if (ImGui::BeginTabBar("Buttons")) {
		for (size_t i = 0; i < buttons_.size(); ++i) {
			if (ImGui::BeginTabItem((std::string("Button ") + std::to_string(i)).c_str())) {
				buttons_[i]->ShowGui();
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}

	ImGui::End();
}

std::vector<Sprite*> TitleMenuController::GetAllButtonImage() const {
	std::vector<Sprite*> images;
	images.reserve(buttons_.size());
	for (const auto& button : buttons_) {
		images.push_back(button->GetSprite());
	}
	return images;
}

void TitleMenuController::SetMenuEvent(std::function<void()> gameStart) {
	if (!buttons_.empty()) {
		buttons_[0]->SetOnExecute(std::move(gameStart)); // ゲームスタートボタン
	}
}

void TitleMenuController::SetGameEndEvent(std::function<void()> gameEnd) {
	if (!buttons_.empty()) {
		buttons_[1]->SetOnExecute(std::move(gameEnd)); // ゲームスタートボタン
	}
}

// GUIから呼ばれる既存関数。現在の animT を使って再配置するように変更。
void TitleMenuController::AdaptationForSprite() {
	LayoutButtons_();
}

void TitleMenuController::LayoutButtons_() {
	CalyxMath::Vector2 pos = config_.basePos_;
	for (size_t i = 0; i < buttons_.size(); ++i) {
		auto* s = buttons_[i]->GetSprite();
		if (!s) { pos.y += config_.space_; continue; }

		// easing
		const float t = (i < selectedAnimT_.size()) ? selectedAnimT_[i] : 0.0f;
		const float eased = config_.useBackEase_ ? CalyxEase::EaseOutBack(t) : CalyxEase::EaseOutQuad(t);

		const float scale = 1.0f + (config_.enlargedScale_ - 1.0f) * eased;

		// 拡大後サイズ
		CalyxMath::Vector2 size = { config_.baseSize_.x * scale, config_.baseSize_.y * scale };

		// 中央を合わせるための位置補正
		CalyxMath::Vector2 center = { pos.x + config_.baseSize_.x * 0.5f, pos.y + config_.baseSize_.y * 0.5f };
		CalyxMath::Vector2 topLeft = { center.x - size.x * 0.5f, center.y - size.y * 0.5f };

		s->SetPosition(topLeft);
		s->SetSize(size);

		pos.y +=config_.space_;
	}
}