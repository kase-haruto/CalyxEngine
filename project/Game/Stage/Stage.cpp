#include "Stage.h"
#include <Engine/Application/System/Environment.h>

void Stage::Initialize(float hp, float limitTime) {
	stageHP_ = hp;
	maxHP_	 = hp;
	timer_	 = limitTime;

	hpUI_ = std::make_unique<Sprite>("Textures/white1x1.dds");
	hpUI_->SetColor({1.0f, 0.0f, 0.0f, 1.0f});
	hpUI_->SetAnchorPoint({0.0f, 0.5f});
	hpUI_->SetSize(hpUISize_);
	hpUI_->SetPosition({static_cast<float>(kWindowWidth) * 0.5f - hpUISize_.x * 0.5f, 50.0f});

	numbersSprite_ = std::make_unique<NumbersSprite>(
		"Textures/Numbers", ".png");
	CalyxEngine::Vector2 scorePos = {100.0f, 100.0f};
	numbersSprite_->Initialize(/*pos*/ scorePos,
							   /*digitSize*/ {32.0f, 32.0f});
	numbersSprite_->SetAlign(NumbersSprite::DigitsAlign::Left);
}

void Stage::Update(float dt) {
	if(timer_ > 0.0f) {
		timer_ -= dt;
	} else {
		timer_ = 0.0f;
		//isGameOver_ = true;
	}
	
	if(numbersSprite_) {
		numbersSprite_->SetValue(int(timer_));
		numbersSprite_->Update();
	}
	if(hpUI_) {
		hpUI_->Update();
	}
}

void Stage::Draw(SpriteRenderer* renderer) {
	if(numbersSprite_) {
		numbersSprite_->Draw(renderer);
	}
	if(hpUI_) {
		renderer->Register(hpUI_.get());
	}
}

void Stage::TakeDamage(float damage) {
	stageHP_ -= damage;
	if(stageHP_ < 0.0f) {
		stageHP_ = 0.0f;
		isClear_ = true;
	}

	float rate = stageHP_ / maxHP_;
	hpUI_->SetSize({hpUISize_.x * rate, hpUISize_.y});
}
