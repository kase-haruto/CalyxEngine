#include "NumbersSprite.h"

// engine
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

NumbersSprite::NumbersSprite() {
	// 何も指定されていないときはデフォルトで1桁0を表示
	std::unique_ptr<Sprite> sprite = std::make_unique<Sprite>(dir_ + "0" + ext_);
	sprite->Initialize(); // 取り合えずwindow中心で初期化

	sprites_.push_back(std::move(sprite));
}

NumbersSprite::NumbersSprite(int value) : value_(value) {
}

NumbersSprite::~NumbersSprite() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void NumbersSprite::Update() {
	// スプライトの更新
	for (auto& sp : sprites_)
		sp->Update();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デバッグgui
/////////////////////////////////////////////////////////////////////////////////////////
void NumbersSprite::ShowGui(const std::string& label) {
	ImGui::Begin(label.c_str());

	ImGui::End();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		getter
/////////////////////////////////////////////////////////////////////////////////////////
const Vector2& NumbersSprite::GetSpriteSize() const		{ return spriteSize_; }
const float	   NumbersSprite::GetValue() const			{ return value_; }
const float	   NumbersSprite::GetSpace() const			{ return space_; }

/////////////////////////////////////////////////////////////////////////////////////////
//		setter
/////////////////////////////////////////////////////////////////////////////////////////
void NumbersSprite::SetAnchor(const Vector2& anc)		{ anchor_ = anc; }
void NumbersSprite::SetSpriteSize(const Vector2& size)	{ spriteSize_ = size; }
void NumbersSprite::SetValue(int val)					{ value_ = val; }
void NumbersSprite::SetMinDigit(int digit)				{ minDigits_ = digit; }
void NumbersSprite::SetMaxDigit(int digit)				{ maxDigits_ = digit; }
void NumbersSprite::SetSpace(float space)				{ space_ = space; }
