#include "ScreenDimFilter.h"

#include <Engine/Renderer/Sprite/SpriteRenderer.h>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// ctor / dtor
///////////////////////////////////////////////////////////////////////////////
ScreenDimFilter::ScreenDimFilter() = default;
ScreenDimFilter::~ScreenDimFilter() = default;

///////////////////////////////////////////////////////////////////////////////
// 初期化
///////////////////////////////////////////////////////////////////////////////
void ScreenDimFilter::Initialize(const CalyxMath::Vector2& screenSize) {
	// Config
	configData_ = std::make_unique<DimFilterConfig>();
	configData_->LoadParams();

	sprite_ = std::make_unique<Calyx2D::SpriteObject2d>();
	sprite_->Initialize("Textures/white1x1.dds");

	sprite_->SetScale(screenSize);

	SetAlpha(configData_->startAlpha);
}

///////////////////////////////////////////////////////////////////////////////
// 更新
///////////////////////////////////////////////////////////////////////////////
void ScreenDimFilter::Update(float dt) {
	if(!isFading_) return;
	sprite_->Update(dt);
	fadeTimer_.Update(dt);

	float t = std::clamp(fadeTimer_.t_, 0.0f, 1.0f);
	float alpha =
		configData_->startAlpha +
		(configData_->targetAlpha - configData_->startAlpha) * t;

	SetAlpha(alpha);

	if(fadeTimer_.IsReached()) {
		isFading_ = false;
		isFinished_ = true;
		SetAlpha(configData_->targetAlpha);
	}
}

///////////////////////////////////////////////////////////////////////////////
// 描画
///////////////////////////////////////////////////////////////////////////////
void ScreenDimFilter::Draw(SpriteRenderer* renderer) const {
	if(sprite_) {
		sprite_->Draw(renderer);
	}
}

///////////////////////////////////////////////////////////////////////////////
// フェードイン
///////////////////////////////////////////////////////////////////////////////
void ScreenDimFilter::StartFadeIn() {
	configData_->startAlpha  = currentAlpha_;
	configData_->targetAlpha = configData_->targetAlpha; // JSON値

	fadeTimer_.target_ = configData_->duration;
	fadeTimer_.Reset();

	isFading_ = true;
}

///////////////////////////////////////////////////////////////////////////////
// フェードアウト
///////////////////////////////////////////////////////////////////////////////
void ScreenDimFilter::StartFadeOut() {
	configData_->startAlpha  = currentAlpha_;
	configData_->targetAlpha = 0.0f;

	fadeTimer_.target_ = configData_->duration;
	fadeTimer_.Reset();

	isFading_ = true;
}

///////////////////////////////////////////////////////////////////////////////
// 即時設定
///////////////////////////////////////////////////////////////////////////////
void ScreenDimFilter::SetAlpha(float alpha) {
	currentAlpha_ = std::clamp(alpha, 0.0f, 1.0f);

	sprite_->SetColor({0.0f, 0.0f, 0.0f, currentAlpha_});
}