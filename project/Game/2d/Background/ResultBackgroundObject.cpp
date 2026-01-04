#include "ResultBackgroundObject.h"

#include "Engine/Foundation/Utility/Func/CxUtils.h"
#include "Engine/Renderer/Sprite/Sprite.h"

namespace {
	//===================================================================*/
	//			private structs / enums
	//===================================================================*/
	enum class ObjectType {
		Top = 0,
		Bottom,
	};
}

ResultBackgroundObject::ResultBackgroundObject()  = default;
ResultBackgroundObject::~ResultBackgroundObject() = default;

void ResultBackgroundObject::Initialize(const std::string& filePath) {

	configData_ = std::make_unique<ResultBackgroundObjectConfig>();
	configData_->LoadParams();

	for(int i = 0; i < 2; ++i) {

		// Sprite
		backgroundSprites_[i] = std::make_unique<Calyx2D::SpriteObject2d>();
		backgroundSprites_[i]->Initialize(filePath);
		backgroundSprites_[i]->SetScale({2048.0f, 1024.0f});
		backgroundSprites_[i]->GetSprite()->SetAnchorPoint({.5f, .5f});

		// Animation
		moveAnims_[i] = std::make_unique<
			CalyxUtil::SimpleAnimation<CalyxMath::Vector2>>();
	}

	// ======================
	// Top
	// ======================
	{
		auto& anim = *moveAnims_[0];
		anim.SetStart(CalyxMath::Vector2{-2000.0f,150.0f});
		anim.SetEnd(CalyxMath::Vector2{400.0f,-400.0f});
		anim.SetDuration(0.3f);
		anim.SetEasing(CalyxEase::EaseType::EaseOutSine);
		anim.SetLoopCount(1);
		anim.Start();

		backgroundSprites_[0]->SetPosition(configData_->topPosition);
	}

	// ======================
	// Bottom
	// ======================
	{
		auto& anim = *moveAnims_[1];
		anim.SetStart(CalyxMath::Vector2{3000.0f,650.0f});
		anim.SetEnd(CalyxMath::Vector2{1200.0f,1050.0f});
		anim.SetDuration(0.3f);
		anim.SetEasing(CalyxEase::EaseType::EaseOutSine);
		anim.SetLoopCount(1);
		anim.Start();

		backgroundSprites_[1]->SetPosition(configData_->bottomPosition);
	}

}

void ResultBackgroundObject::Update(float dt) {

	const CalyxMath::Vector2 uvScroll   = { configData_->uvScrollSpeed * dt,0.0f};
	const float              uvRotDelta = configData_->uvRotate * dt;

	for(int i = 0; i < 2; ++i) {
		auto& sprite = *backgroundSprites_[i];
		auto& anim   = *moveAnims_[i];

		// Position
		CalyxMath::Vector2 pos = sprite.GetPosition();
		anim.LerpValue(pos,dt);
		sprite.SetPosition(pos);
		sprite.SetRotation(configData_->topRotate);
		// UV Scroll / UV Rotate
		// ボトムは反対方向にスクロール
		CalyxMath::Vector2 uvSpeed;
		if (i == static_cast<int>(ObjectType::Bottom)) {
			uvSpeed = { -configData_->uvScrollSpeed * dt,0.0f }; // 反対方向
		} else {
			uvSpeed = { configData_->uvScrollSpeed * dt,0.0f }; // 反対方向
		}
		float uvX   = sprite.GetUvTranslate().x - uvSpeed.x * dt;
		sprite.SetUvTranslate({uvX, sprite.GetUvTranslate().y});
		sprite.SetUvRotate(sprite.GetUvRotate() + uvRotDelta); // 絶対値方式

		sprite.Update(dt);
	}
}

void ResultBackgroundObject::Draw(SpriteRenderer* renderer) const {
	for(const auto& sprite : backgroundSprites_) { sprite->Draw(renderer); }
}

void ResultBackgroundObject::ShowGui() {

	if(!configData_) return;

	// Config
	if(ImGui::CollapsingHeader("Config",ImGuiTreeNodeFlags_DefaultOpen)) { configData_->SaveAndLoadButtonGui(); }

	configData_->ShowGui();

	// Animation
	if(ImGui::CollapsingHeader("Move Animation",
							   ImGuiTreeNodeFlags_DefaultOpen)) {
		moveAnims_[0]->ImGui("Top",true);
		ImGui::Separator();
		moveAnims_[1]->ImGui("Bottom",true);
	}

	// Control
	if(ImGui::CollapsingHeader("Control")) {
		if(ImGui::Button("Start")) {
			moveAnims_[0]->Start();
			moveAnims_[1]->Start();
		}
		ImGui::SameLine();

		if(ImGui::Button("Reset")) {
			moveAnims_[0]->Reset();
			moveAnims_[1]->Reset();
		}
	}

}