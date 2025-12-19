#pragma once

// engine
#include <Engine/Renderer/Sprite/Sprite.h>

// c++
#include <string>
#include <functional>
#include <memory>

struct CxMath::Vector2;

class Button {
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	Button(const std::string& texturePath, const CxMath::Vector2& pos, const CxMath::Vector2& size);

	void Update(float dt);
	void SetOnExecute(std::function<void()>cb);
	void SetSelected(bool selected);
	void ShowGui();
	void Execute();
	Sprite* GetSprite();

private:
	//===================================================================*/
	//						private methods
	//===================================================================*/
	std::unique_ptr<Sprite> image_;		// ボタン表示画像
	std::function<void()> onExecute_;
	CxMath::Vector2 pos_;
	CxMath::Vector2 size_;
	bool isSelected_ = false;
};

