#include "SpriteObject2d.h"

#include "Engine/Renderer/Sprite/SpriteRenderer.h"

#include <Engine/Renderer/Sprite/Sprite.h>

using namespace Calyx2D;

SpriteObject2d::SpriteObject2d()  = default;
SpriteObject2d::~SpriteObject2d() = default;

void SpriteObject2d::Initialize(const std::string& filePath) {
	sprite_ = std::make_unique<Sprite>(filePath);
	sprite_->Initialize();
}

void SpriteObject2d::Update(float dt) {
	if(currentAnim_ && division_.first > 1 && division_.second >= 1) {
		AnimationUpdate(dt);
	}
	sprite_->Update();
}

void SpriteObject2d::Draw(SpriteRenderer* renderer) const {
	renderer->Register(sprite_.get());
}

void SpriteObject2d::ShowGui() {
	sprite_->ShowGui();
}

//====================================================
// アニメ追加
//====================================================
void SpriteObject2d::AddAnimation(const std::string& name, SpriteAnimation anim) {
	// frameCount が 0 の場合に自動計算
	if(anim.frameCount == 0) {
		anim.frameCount = anim.division.first * anim.division.second;
	}

	animations_[name] = anim;
}

//====================================================
// アニメ切替
//====================================================
bool SpriteObject2d::SetAnimation(const std::string& name, bool restart) {
	auto it = animations_.find(name);
	if(it == animations_.end()) return false;

	currentAnim_	 = &it->second;
	currentAnimName_ = name;

	// テクスチャ切替
	sprite_->SetTexture(currentAnim_->texturePath);

	// division更新
	division_ = currentAnim_->division;

	// 再生位置初期化
	if(restart) {
		currentFrame_ = currentAnim_->startFrame;
		frameTime_	  = 0.0f;
	}

	// UV反映
	ApplyFrameToUv();
	return true;
}

//====================================================
// アニメ更新
//====================================================
void SpriteObject2d::AnimationUpdate(float dt) {
	if(!currentAnim_) return;
	if(division_.first < 1 || division_.second < 1) return;

	frameTime_ += dt;
	if(frameTime_ >= currentAnim_->frameDuration) {

		frameTime_ -= currentAnim_->frameDuration;
		currentFrame_++;

		int endFrame	= currentAnim_->startFrame + currentAnim_->frameCount;

		// 終了判定
		if(currentFrame_ >= endFrame) {
			if(currentAnim_->loop) {
				currentFrame_ = currentAnim_->startFrame;
			} else {
				currentFrame_ = endFrame - 1; // 最後で停止
			}
		}

		ApplyFrameToUv();
	}
}

//====================================================
// UV反映
//====================================================
void SpriteObject2d::ApplyFrameToUv() const {

	int divX = division_.first;
	int divY = division_.second;

	int fx = currentFrame_ % divX;
	int fy = currentFrame_ / divX;

	float frameW = 1.0f / divX;
	float frameH = 1.0f / divY;

	sprite_->SetUvScale({frameW, frameH});
	sprite_->SetUvOffset({fx * frameW, fy * frameH});
}

//====================================================
// getter / setter
//====================================================
const std::pair<int32_t, int32_t>& SpriteObject2d::GetDivision() const { return division_; }
const CalyxMath::Vector2&		   SpriteObject2d::GetPosition() const { return sprite_->GetPosition(); }
const CalyxMath::Vector2&		   SpriteObject2d::GetScale() const { return sprite_->GetSize(); }
Sprite*							   SpriteObject2d::GetSprite() const { return sprite_.get(); }
CalyxMath::Vector2 SpriteObject2d::GetUvTranslate() const {
	return sprite_->GetUvTranslate();
}
float SpriteObject2d::GetUvRotate() const {
	return sprite_->GetUvRotate();
}

void SpriteObject2d::SetDivision(const std::pair<int32_t, int32_t>& division) { division_ = division; }
void SpriteObject2d::SetPosition(const CalyxMath::Vector2& position) const { sprite_->SetPosition(position); }
void SpriteObject2d::SetScale(const CalyxMath::Vector2& scale) const { sprite_->SetSize(scale); }
void SpriteObject2d::SetRotation(float rotation) const {sprite_->SetRotation(rotation); }
void SpriteObject2d::SetAlpha(float alpha) const { sprite_->SetAlpha(alpha); }
void SpriteObject2d::SetColor(const CalyxMath::Vector4& color) const {sprite_->SetColor(color);}
void SpriteObject2d::SetVisibility(bool visible) const {sprite_->SetIsVisible(visible);}


void SpriteObject2d::SetUvTranslate(const CalyxMath::Vector2& uv) const {
	sprite_->SetUvTranslate(uv);
}
void SpriteObject2d::SetUvRotate(float rot) const {
	sprite_->SetUvRotate(rot);
}
void SpriteObject2d::SetUvOffset(const CalyxMath::Vector2& offset) const {
	sprite_->SetUvOffset(offset);
}
void SpriteObject2d::SetFrameDuration(float duration)  {
	frameDuration_=duration;
}