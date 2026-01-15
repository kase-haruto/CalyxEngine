#pragma once
#include "Data/Game/Config/Title/TitleMenueConfig.h"

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
	TitleMenueConfig config_;

	// 選択状態
	uint16_t selectedIndex_ = 0;
	std::vector<std::unique_ptr<Button>> buttons_;

	// ── 追加：選択拡大アニメ用パラメータ ────────────────────────────
	// t: 0→非選択, 1→選択。毎フレーム target に向けて補間
	std::vector<float> selectedAnimT_;

	// レイアウト計算（毎フレーム呼ぶ）
	void LayoutButtons_();
};