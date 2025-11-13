#include "HpGauge.h"

#include "Engine/Foundation/Utility/Ease/Ease.h"

#include <Engine/Foundation/Utility/Ease/CxEase.h>
#include <algorithm>
#include <cmath>

HpGauge::HpGauge(float maxHp)
	: maxHp_(maxHp),
	  currentHp_(maxHp),
	  blueRatio_(1.0f),
	  redRatio_(1.0f) {}

HpGauge::~HpGauge() = default;


/////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////
void HpGauge::Initialize(const Vector2& position,const Vector2& size) {
	baseSize_ = size;

	// 青ゲージ（即時更新）
	blueGauge_ = std::make_unique<Sprite>("Textures/white1x1.png");
	blueGauge_->Initialize(position,size);

	// 赤ゲージ（遅延追従）
	redGauge_ = std::make_unique<Sprite>("Textures/white1x1.png");
	redGauge_->Initialize(position,size);

	// フレーム
	frameSprite_ = std::make_unique<Sprite>("Textures/white1x1.png");
	frameSprite_->Initialize(position,size);
}


/////////////////////////////////////////////////////////////////////////////////////////
//		ゲーム側がhpをセット
/////////////////////////////////////////////////////////////////////////////////////////
void HpGauge::SetHp(float hp) {
	currentHp_ = std::clamp(hp,0.0f,maxHp_);

	// 変動があったら可視時間をリセット
	visibleTimer_ = 1.0f;

	// HP 減少 → バウンド演出
	shakeTimer_ = 0.3f;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void HpGauge::Update(float dt) {(void)dt;}

//------------------------------------------------------------
// HP 比率による色変化（緑→黄→赤）
//------------------------------------------------------------
Vector4 HpGauge::ComputeColor(float ratio) const {
	if(ratio > 0.5f) {
		// 緑 → 黄
		float t = (ratio - 0.5f) / 0.5f;
		return {1 - t,1.0f,0.0f,1.0f};
	} else {
		// 黄 → 赤
		float t = ratio / 0.5f;
		return {1.0f,t,0.0f,1.0f};
	}
}