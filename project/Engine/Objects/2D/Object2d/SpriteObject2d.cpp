#include "SpriteObject2d.h"

#include <Engine/Renderer/Sprite/Sprite.h>

Calyx2D::SpriteObject2d::SpriteObject2d()  = default;
Calyx2D::SpriteObject2d::~SpriteObject2d() = default;

///////////////////////////////////////////////////////////////////////////////////////////
//  初期化処理
///////////////////////////////////////////////////////////////////////////////////////////
void Calyx2D::SpriteObject2d::Initializes(const std::string& filePath) {
	sprite_ = std::make_unique<Sprite>(filePath);
	sprite_->Initialize();
}

///////////////////////////////////////////////////////////////////////////////////////////
//  更新処理
///////////////////////////////////////////////////////////////////////////////////////////
void Calyx2D::SpriteObject2d::Update(float dt) const {
	AnimationUpdate(dt);
	sprite_->Update();
}

///////////////////////////////////////////////////////////////////////////////////////////
//  描画処理
///////////////////////////////////////////////////////////////////////////////////////////
void Calyx2D::SpriteObject2d::Draw(ID3D12GraphicsCommandList* cmdList) const {
	sprite_->Draw(cmdList);
}

///////////////////////////////////////////////////////////////////////////////////////////
//  アニメーションがあれば更新
///////////////////////////////////////////////////////////////////////////////////////////
void Calyx2D::SpriteObject2d::AnimationUpdate(float dt) const {
	if(division_.first < 1 || division_.second < 1) return;

	// 時間更新
	frameTime_ += dt;
	if(frameTime_ >= frameDuration_) {
		frameTime_ -= frameDuration_;
		currentFrame_ = (currentFrame_ + 1) % (division_.first * division_.second);
	}

	// フレーム位置
	int divX = division_.first;
	int divY = division_.second;

	int fx = currentFrame_ % divX;
	int fy = currentFrame_ / divX;

	float frameW = 1.0f / divX;
	float frameH = 1.0f / divY;

	// UV セット
	sprite_->SetUvScale({frameW, frameH});
	sprite_->SetUvOffset({fx * frameW, fy * frameH});
}

///////////////////////////////////////////////////////////////////////////////////////////
//  getter
///////////////////////////////////////////////////////////////////////////////////////////
const std::pair<int32_t, int32_t>& Calyx2D::SpriteObject2d::GetDivision() const { return division_; }
const CalyxMath::Vector2&		   Calyx2D::SpriteObject2d::GetPosition() const { return sprite_->GetPosition(); }
const CalyxMath::Vector2&		   Calyx2D::SpriteObject2d::GetScale() const { return sprite_->GetSize(); }
Sprite*							   Calyx2D::SpriteObject2d::GetSprite() const {return sprite_.get();}

///////////////////////////////////////////////////////////////////////////////////////////
//  settere
///////////////////////////////////////////////////////////////////////////////////////////
void Calyx2D::SpriteObject2d::SetDivision(const std::pair<int32_t, int32_t>& division) { division_ = division; }
void Calyx2D::SpriteObject2d::SetPosition(const CalyxMath::Vector2& position) const { sprite_->SetPosition(position); }
void Calyx2D::SpriteObject2d::SetScale(const CalyxMath::Vector2& scale) const { sprite_->SetSize(scale); }
