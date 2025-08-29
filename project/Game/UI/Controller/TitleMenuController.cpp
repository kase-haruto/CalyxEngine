#include "TitleMenuController.h"

// engine
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine/Foundation/Math/Vector2.h>

// externals
#include <externals/imgui/imgui.h>


TitleMenuController::TitleMenuController() :
	basePos_(Vector2(750, 400.0f)),
	baseSize_(Vector2(256.0f, 64.0f)),
	space_(100.0f) {

	// ゲームスタートボタン
	std::unique_ptr<Button> startButton =
		std::make_unique<Button>("Textures/white1x1.png",
								 basePos_,
								 baseSize_);
	basePos_.y += space_;

	std::unique_ptr<Button> exitButton =
		std::make_unique<Button>("Textures/white1x1.png",
								 basePos_,
								 baseSize_);

	//リストに追加
	buttons_.push_back(std::move(startButton));
	buttons_.push_back(std::move(exitButton));
}

void TitleMenuController::Update(float dt) {
	for (auto& button : buttons_) {
		button->Update(dt);
	}
}

void TitleMenuController::ShowGui() {
	ImGui::Begin("menuController");

	ImGui::SeparatorText("baseParm");
	bool changed = false;
	changed |= GuiCmd::DragFloat2("basePos", basePos_);
	changed |= GuiCmd::DragFloat2("baseSize", baseSize_);
	changed |= GuiCmd::DragFloat("space", space_);

	if (changed) AdaptationForSprite(); // ← 追加

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

void TitleMenuController::AdaptationForSprite() {
	Vector2 pos = basePos_;
	for (auto& b : buttons_) {
		if (auto* s = b->GetSprite()) {
			s->SetPosition(pos);
			s->SetSize(baseSize_);
		}
		pos.y += space_;
	}
}