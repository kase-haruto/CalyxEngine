#include "HpGauge.h"

// imgui
#include "imgui/imgui.h"

// ease
#include <Engine/Foundation/Utility/Ease/CxEase.h>

#include <algorithm>
#include <cmath>

using CalyxEase::EaseType;

namespace {

/////////////////////////////////////////////////////////////////////////////////////////
// HP 比率による色変化（緑→黄→赤）→ 青ゲージ(現在HP)に使用
/////////////////////////////////////////////////////////////////////////////////////////
CalyxMath::Vector4 ComputeColor(float ratio)
{
	if (ratio > 0.5f) {
		float t = (ratio - 0.5f) / 0.5f;
		return {1.0f - t, 1.0f, 0.0f, 1.0f};  // 緑→黄
	} else {
		float t = ratio / 0.5f;
		return {1.0f, t, 0.0f, 1.0f};        // 黄→赤
	}
}

} // namespace


/////////////////////////////////////////////////////////////////////////////////////////
// コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
HpGauge::HpGauge(float maxHp)
	: maxHp_(maxHp)
	, currentHp_(maxHp)
	, blueRatio_(1.0f)   // 現在HP
	, redRatio_(1.0f)    // 遅延
{
}

HpGauge::~HpGauge() = default;


/////////////////////////////////////////////////////////////////////////////////////////
// 初期化
/////////////////////////////////////////////////////////////////////////////////////////
void HpGauge::Initialize(const CalyxMath::Vector2& position, const CalyxMath::Vector2& size)
{
	baseSize_ = size;

	transform_.Initialize();
	transform_.translate = position;
	transform_.scale     = size;

	//----------------------------------------
	// 青ゲージ（現在 HP）
	//----------------------------------------
	blueGauge_ = std::make_unique<Sprite>("Textures/white1x1.png");
	blueGauge_->Initialize(position, size);
	blueGauge_->SetFillMethod(1);
	blueGauge_->SetFillOrigin(0.0f, 0.0f);
	blueGauge_->SetFillAmount(1.0f);

	//----------------------------------------
	// 赤ゲージ（遅延）
	//----------------------------------------
	redGauge_ = std::make_unique<Sprite>("Textures/white1x1.png");
	redGauge_->Initialize(position, size);
	redGauge_->SetFillMethod(1);
	redGauge_->SetFillOrigin(0.0f, 0.0f);
	redGauge_->SetFillAmount(1.0f);
	redGauge_->SetColor(CalyxMath::Vector4(1,0,0,1));

	//----------------------------------------
	// フレーム
	//----------------------------------------
	frameSprite_ = std::make_unique<Sprite>("Textures/white1x1.png");
	frameSprite_->Initialize(position, size);
	frameSprite_->SetFillMethod(0);
}


/////////////////////////////////////////////////////////////////////////////////////////
// HP 設定
/////////////////////////////////////////////////////////////////////////////////////////
void HpGauge::SetHp(float hp)
{
	currentHp_ = std::clamp(hp, 0.0f, maxHp_);
	visibleTimer_ = 1.0f;

	// 青ゲージ（現在HP）を基準に減少チェック
	float prevBlue = blueRatio_ * maxHp_;
	if (currentHp_ < prevBlue)
	{
		shakeTimer_ = 0.3f;
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
// GUI
/////////////////////////////////////////////////////////////////////////////////////////
void HpGauge::ShowGui(const std::string& label)
{
	ImGui::Text("HP : %.1f / %.1f", currentHp_, maxHp_);
	ImGui::Text("blueRatio (current HP) : %.2f", blueRatio_);
	ImGui::Text("redRatio  (delayed)    : %.2f", redRatio_);

	transform_.ShowImGui(label + "_transform");

	blueGauge_->ShowGui();
}


/////////////////////////////////////////////////////////////////////////////////////////
// 更新
/////////////////////////////////////////////////////////////////////////////////////////
void HpGauge::Update(float dt)
{
	float invMaxHp = (maxHp_ > 0.0f ? 1.0f / maxHp_ : 0.0f);
	float target   = currentHp_ * invMaxHp;

	blueRatio_ = CalyxEase::EaseLerp(
		blueRatio_,
		target,
		dt * 12.0f,
		EaseType::EaseOutSine
	);

	redRatio_ = CalyxEase::EaseLerp(
		redRatio_,
		blueRatio_,
		dt * 2.0f,      // target に直接追従させない
		EaseType::EaseOutExpo
	);
	//----------------------------------------
	// バウンド演出
	//----------------------------------------
	float scaleY = 1.0f;
	if (shakeTimer_ > 0.0f) {
		shakeTimer_ -= dt;
		float elapsed = 0.3f - (std::max)(shakeTimer_, 0.0f);
		scaleY = 1.0f + 0.2f * std::sin(elapsed * 20.0f);
	}
	transform_.scale.y = baseSize_.y * scaleY;

	//----------------------------------------
	// フェード
	//----------------------------------------
	if (visibleTimer_ > 0.0f) visibleTimer_ -= dt;
	float alpha = std::clamp(visibleTimer_ * 2.0f, 0.3f, 1.0f);

	//----------------------------------------
	// transform 同期（位置・サイズ・回転）
	//----------------------------------------
	SyncTransform();

	//----------------------------------------
	// 青ゲージ（現在 HP）
	//----------------------------------------
	if (blueGauge_) {
		// uv スクロール
		float speed = 2.0f;
		float uvY   = blueGauge_->GetUvTranslate().y - speed * dt;
		blueGauge_->SetUvTranslate({uvY, uvY});
		blueGauge_->SetFillAmount(blueRatio_);
		blueGauge_->SetColor(ComputeColor(blueRatio_));
		blueGauge_->SetAlpha(alpha);
		blueGauge_->Update();
	}

	//----------------------------------------
	// 赤ゲージ（遅延）
	//----------------------------------------
	if (redGauge_) {
		redGauge_->SetFillAmount(redRatio_);
		redGauge_->SetAlpha(alpha);
		redGauge_->Update();
	}

	//----------------------------------------
	// フレーム
	//----------------------------------------
	if (frameSprite_) {
		frameSprite_->SetAlpha(alpha);
		frameSprite_->Update();
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
// anchor
/////////////////////////////////////////////////////////////////////////////////////////
void HpGauge::SetAncorPoint(const CalyxMath::Vector2& point) const
{
	if (blueGauge_) blueGauge_->SetAnchorPoint(point);
	if (redGauge_) redGauge_->SetAnchorPoint(point);
	if (frameSprite_) frameSprite_->SetAnchorPoint(point);
}


/////////////////////////////////////////////////////////////////////////////////////////
// transform 同期
/////////////////////////////////////////////////////////////////////////////////////////
void HpGauge::SyncTransform() const
{
	// 描画順とは無関係。全て同じ transform を当てる。
	if (blueGauge_) {
		blueGauge_->SetPosition(transform_.translate);
		blueGauge_->SetSize(transform_.scale);
		blueGauge_->SetRotation(transform_.rotate);
	}

	if (redGauge_) {
		redGauge_->SetPosition(transform_.translate);
		redGauge_->SetSize(transform_.scale);
		redGauge_->SetRotation(transform_.rotate);
	}

	if (frameSprite_) {
		frameSprite_->SetPosition(transform_.translate);
		frameSprite_->SetSize(transform_.scale);
		frameSprite_->SetRotation(transform_.rotate);
	}
}