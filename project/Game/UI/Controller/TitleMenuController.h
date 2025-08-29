#pragma once

// engine
#include <Game/UI/Button/Button.h>

// c++
#include <vector>
#include <memory>
#include <cstdint>

class TitleMenuController {
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	TitleMenuController();
	~TitleMenuController() = default;

	void Update(float dt);
	void ShowGui();

	//--------- accessor ---------------------------------------------------
	std::vector<Sprite*> GetAllButtonImage()const;

private:
	void AdaptationForSprite();

private:
	//===================================================================*/
	//						private methods
	//===================================================================*/
	std::vector<std::unique_ptr<Button>> buttons_;
	uint16_t selectedIndex_ = 0;
	
	Vector2 basePos_{};
	Vector2 baseSize_{};
	float space_;
};

