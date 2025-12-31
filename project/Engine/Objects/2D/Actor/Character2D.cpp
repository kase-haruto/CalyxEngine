#include "Character2D.h"
Calyx2D::Character2D::Character2D()	 = default;
Calyx2D::Character2D::~Character2D() = default;

void Calyx2D::Character2D::Update(float dt) {
	if(spriteObj_) spriteObj_->Update(dt);
}
void Calyx2D::Character2D::Draw(SpriteRenderer* renderer) const {
	if(spriteObj_) spriteObj_->Draw(renderer);
}