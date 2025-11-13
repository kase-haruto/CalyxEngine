#pragma once

//engine
#include <Engine/Renderer/Sprite/Sprite.h>

// stl
#include <memory>

// forward declaration
struct Vector2;
class Sprite;

/*********************************************
 * HPゲージをスプライトで表現するクラス
 *********************************************/
class HpGauge {
public:
	HpGauge(float maxHp);
	~HpGauge();

	void Initialize(const Vector2& position,const Vector2& size);
	void Update(float dt);
	void SetHp(float hp); // HP変更 API

	// スプライト取得
	Sprite* GetMainGauge() const { return blueGauge_.get(); }
	Sprite* GetDamageGauge() const { return redGauge_.get(); }
	Sprite* GetFrameSprite() const { return frameSprite_.get(); }

private:
	// 色補間
	Vector4 ComputeColor(float ratio) const;

private:
	// ========= HP =========
	float maxHp_;
	float currentHp_; // ゲーム側が設定する HP（目標）
	float blueRatio_; // 実際の青ゲージの補間値
	float redRatio_;  // 遅れて減る赤ゲージの補間値

	// ========= 演出 =========
	float shakeTimer_   = 0.0f; // バウンド演出
	float visibleTimer_ = 0.0f; // フェード演出

	// ========= サイズ =========
	Vector2 baseSize_;

	// ========= スプライト =========
	std::unique_ptr<Sprite> blueGauge_;   // 青：現在 HP
	std::unique_ptr<Sprite> redGauge_;    // 赤：遅れて減る
	std::unique_ptr<Sprite> frameSprite_; // 枠
};