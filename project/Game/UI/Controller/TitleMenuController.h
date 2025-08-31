#pragma once
#include <memory>
#include <vector>
#include <functional>
#include <cstdint>

#include <Engine/Foundation/Math/Vector2.h>

class Sprite;
class Button;

class TitleMenuController {
public:
	TitleMenuController();
	~TitleMenuController();
	void Update(float dt);
	void ShowGui();

	std::vector<Sprite*> GetAllButtonImage() const;

	void SetMenuEvent(std::function<void()> gameStart);
	void SetGameEndEvent(std::function<void()> gameEnd);

	void AdaptationForSprite();

private:
	// 基本レイアウト
	Vector2 basePos_{ 750.0f, 400.0f };
	Vector2 baseSize_{ 256.0f, 64.0f };
	float   space_{ 100.0f };

	// 選択状態
	uint16_t selectedIndex_ = 0;
	std::vector<std::unique_ptr<Button>> buttons_;

	// ── 追加：選択拡大アニメ用パラメータ ────────────────────────────
	// t: 0→非選択, 1→選択。毎フレーム target に向けて補間
	std::vector<float> selectedAnimT_;
	float enlargedScale_ = 1.20f;	// 選択時の最大倍率（1.0=等倍, 1.2=20%拡大）
	float animSpeed_ = 8.0f;		// t 補間速度（大きいほど速い）
	bool  useBackEase_ = true;		// EaseOutBack か EaseOutQuad か

	// レイアウト計算（毎フレーム呼ぶ）
	void LayoutButtons_();
};
