#include "HpGauge.h"

// ease
#include <Engine/Foundation/Utility/Ease/CxEase.h> // Cx::Ease::EaseLerp / ApplyEase

#include <algorithm>
#include <cmath>

using Cx::Ease::EaseType;
using Cx::Ease::EaseLerp;

HpGauge::HpGauge(float maxHp)
    : maxHp_(maxHp)
    , currentHp_(maxHp)
    , blueRatio_(1.0f)
    , redRatio_(1.0f) {
}

HpGauge::~HpGauge() = default;

//------------------------------------------------------------
// 初期化
//------------------------------------------------------------
void HpGauge::Initialize(const Vector2& position, const Vector2& size) {
    baseSize_ = size;

    // 青ゲージ（現在HP）
    blueGauge_ = std::make_unique<Sprite>("Textures/white1x1.png");
    blueGauge_->Initialize(position, size);
    // 横方向 左→右フィル
    blueGauge_->SetFillMethod(1);      // 1: horizontal
    blueGauge_->SetFillOrigin(0.0f, 0.0f);
    blueGauge_->SetFillAmount(1.0f);

    // 赤ゲージ（遅れて追従するダメージゲージ）
    redGauge_ = std::make_unique<Sprite>("Textures/white1x1.png");
    redGauge_->Initialize(position, size);
    redGauge_->SetFillMethod(1);
    redGauge_->SetFillOrigin(0.0f, 0.0f);
    redGauge_->SetFillAmount(1.0f);

    // フレーム（フィル機能は使わない）
    frameSprite_ = std::make_unique<Sprite>("Textures/white1x1.png");
    frameSprite_->Initialize(position, size);
    frameSprite_->SetFillMethod(0); // 0: none
}

//------------------------------------------------------------
// ゲーム側が HP をセット
//------------------------------------------------------------
void HpGauge::SetHp(float hp) {
    currentHp_ = std::clamp(hp, 0.0f, maxHp_);

    // 変動があったら可視時間をリセット
    visibleTimer_ = 1.0f;

    // HP 減少 → バウンド演出（増加時も揺らしたいなら条件分岐を外す）
    // ここでは「減ったときだけ揺らす」例：
    if (currentHp_ < blueRatio_ * maxHp_) {
        shakeTimer_ = 0.3f;
    }
}

//------------------------------------------------------------
// 更新
//------------------------------------------------------------
void HpGauge::Update(float dt) {
    // 安全対策
    const float invMaxHp = (maxHp_ > 0.0f) ? (1.0f / maxHp_) : 0.0f;
    const float target   = currentHp_ * invMaxHp; // 0〜1

    //========================================
    // 青ゲージ：速く追従（EaseLerp が start→end を補間）
    //========================================
    {
        const float t = dt * 10.0f; // 追従スピード
        blueRatio_ = Cx::Ease::EaseLerp(
            blueRatio_,        // start
            target,            // end
            t,                 // 補間率（0〜1にクランプされる）
            EaseType::EaseOutSine // カーブ形状
        );
    }

    //========================================
    // 赤ゲージ：遅れて追従
    //========================================
    {
        const float t = dt * 3.0f; // ゆっくり追従
        redRatio_ = Cx::Ease::EaseLerp(
            redRatio_,
            target,
            t,
            EaseType::EaseOutCubic
        );
    }

    //========================================
    // バウンド演出（Yスケール）
    //========================================
    float scaleY = 1.0f;
    if (shakeTimer_ > 0.0f) {
        shakeTimer_ -= dt;
        const float elapsed = 0.3f - (std::max)(shakeTimer_, 0.0f);
        scaleY = 1.0f + 0.2f * std::sin(elapsed * 20.0f);
    }

    //========================================
    // フェード制御（HP変化時だけ少し濃く）
    //========================================
    if (visibleTimer_ > 0.0f) {
        visibleTimer_ -= dt;
    }
    float alpha = std::clamp(visibleTimer_ * 2.0f, 0.3f, 1.0f);

    //========================================
    // 青ゲージ更新（fillAmount と色）
    //========================================
    if (blueGauge_) {
        blueGauge_->SetFillAmount(blueRatio_);            // 0〜1
        blueGauge_->SetColor(ComputeColor(blueRatio_));   // 色グラデ
        blueGauge_->SetSize({1.0f, scaleY});
        blueGauge_->SetAlpha(alpha);
        blueGauge_->Update();
    }

    //========================================
    // 赤ゲージ更新
    //========================================
    if (redGauge_) {
        redGauge_->SetFillAmount(redRatio_);
        // ダメージゲージは常に赤
        redGauge_->SetColor({1.0f, 0.0f, 0.0f, 1.0f});
        redGauge_->SetSize({1.0f, scaleY});
        redGauge_->SetAlpha(alpha);
        redGauge_->Update();
    }

    //========================================
    // フレーム更新（スケールは 1、アルファだけ追従）
    //========================================
    if (frameSprite_) {
        frameSprite_->SetAlpha(alpha);
        frameSprite_->Update();
    }
}

//------------------------------------------------------------
// HP 比率による色変化（緑→黄→赤）
//------------------------------------------------------------
Vector4 HpGauge::ComputeColor(float ratio) const {
    // ratio は 0〜1 の前提（blueRatio_ をそのまま渡す）
    if (ratio > 0.5f) {
        // 緑 → 黄
        float t = (ratio - 0.5f) / 0.5f;
        return {1.0f - t, 1.0f, 0.0f, 1.0f};
    } else {
        // 黄 → 赤
        float t = ratio / 0.5f;
        return {1.0f, t, 0.0f, 1.0f};
    }
}