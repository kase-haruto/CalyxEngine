#include "Character2D.h"
CalyxEngine::Character2D::Character2D()	 = default;
CalyxEngine::Character2D::~Character2D() = default;

void CalyxEngine::Character2D::Update(float dt) {
	if(spriteObj_) spriteObj_->Update(dt);
}
void CalyxEngine::Character2D::Draw(SpriteRenderer* renderer) const {
	if(spriteObj_) spriteObj_->Draw(renderer);
}