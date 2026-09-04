#include "SpriteObject2d.h"

#include "Engine/Renderer/Sprite/SpriteRenderer.h"

#include <Engine/Renderer/Sprite/Sprite.h>

using namespace CalyxEngine;

SpriteObject2d::SpriteObject2d() {
	SetName("Sprite2D", ObjectType::Object2D);
	SetCastShadow(false);
	// Instantiateの戻り値から直ちに設定できるよう、既定Spriteを生成しておく。
	Initialize();
}
SpriteObject2d::~SpriteObject2d() = default;

void SpriteObject2d::Initialize() {
	if(!sprite_) {
		Initialize("Textures/white1x1.dds");
	}
}

void SpriteObject2d::Initialize(const std::string& filePath) {
	sprite_ = std::make_unique<Sprite>(filePath);
	sprite_->Initialize();
}

void SpriteObject2d::Update(float dt) {
	(void)dt;
	sprite_->Update();
}

void SpriteObject2d::Draw(SpriteRenderer* renderer) const {
	if(renderer) {
		SubmitSprites(*renderer);
	}
}

void SpriteObject2d::SubmitSprites(SpriteRenderer& renderer) const {
	if(sprite_ && IsDrawEnable()) {
		renderer.Register(sprite_.get());
	}
}

void SpriteObject2d::ShowGui() {
	if(sprite_) sprite_->ShowGui();
}

//====================================================
// getter / setter
//====================================================
const CalyxEngine::Vector2&		   SpriteObject2d::GetPosition() const { return sprite_->GetPosition(); }
const CalyxEngine::Vector2&		   SpriteObject2d::GetScale() const { return sprite_->GetSize(); }
Sprite*							   SpriteObject2d::GetSprite() const { return sprite_.get(); }
CalyxEngine::Vector2 SpriteObject2d::GetUvTranslate() const {
	return sprite_->GetUvTranslate();
}
float SpriteObject2d::GetUvRotate() const {
	return sprite_->GetUvRotate();
}

void SpriteObject2d::SetTexture(const std::string& texturePath) const { sprite_->SetTexture(texturePath); }
bool SpriteObject2d::SetTextureSheet(const std::string& texturePath, int32_t columns, int32_t rows) const {
	return sprite_->SetTextureSheet(texturePath, columns, rows);
}
bool SpriteObject2d::SetTextureGrid(int32_t columns, int32_t rows) const {
	return sprite_->SetTextureGrid(columns, rows);
}
void SpriteObject2d::SetTextureFrame(int32_t frame) const {
	sprite_->SetTextureFrame(frame);
}
void SpriteObject2d::SetPosition(const CalyxEngine::Vector2& position) const { sprite_->SetPosition(position); }
void SpriteObject2d::SetScale(const CalyxEngine::Vector2& scale) const { sprite_->SetSize(scale); }
void SpriteObject2d::SetRotation(float rotation) const {sprite_->SetRotation(rotation); }
void SpriteObject2d::SetAlpha(float alpha) const { sprite_->SetAlpha(alpha); }
void SpriteObject2d::SetColor(const CalyxEngine::Vector4& color) const {sprite_->SetColor(color);}
void SpriteObject2d::SetVisibility(bool visible) const {sprite_->SetIsVisible(visible);}
void SpriteObject2d::SetAnchorPoint(const CalyxEngine::Vector2& anchor) const { sprite_->SetAnchorPoint(anchor); }


void SpriteObject2d::SetUvTranslate(const CalyxEngine::Vector2& uv) const {
	sprite_->SetUvTranslate(uv);
}
void SpriteObject2d::SetUvRotate(float rot) const {
	sprite_->SetUvRotate(rot);
}
void SpriteObject2d::SetUvScale(const CalyxEngine::Vector2& scale) const {
	sprite_->SetUvScale(scale);
}
void SpriteObject2d::SetUvOffset(const CalyxEngine::Vector2& offset) const {
	sprite_->SetUvOffset(offset);
}

void CalyxEngine::SpriteObject2d::SetFillAmount(float fillAmount) const {
	sprite_->SetFillAmount(fillAmount);
}

void CalyxEngine::SpriteObject2d::SetFillOrigin(float x, float y) {
	sprite_->SetFillOrigin(x, y);
}

void CalyxEngine::SpriteObject2d::SetFillMethod(int method) {
	sprite_->SetFillMethod(method);
}
