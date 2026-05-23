#include "NumbersSprite.h"
#include <algorithm>
#include <cmath>

static inline std::string Join2_(const std::string& a,const std::string& b) {
	const char sep = '/';
	if(a.empty()) return b;
	if(a.back() == '/' || a.back() == '\\') return a + b;
	return a + sep + b;
}

std::string NumbersSprite::JoinPath_(const std::string& a,const std::string& b) { return Join2_(a,b); }

NumbersSprite::NumbersSprite(std::string dir,std::string ext)
	: dir_(std::move(dir)), ext_(std::move(ext)) {}

NumbersSprite::NumbersSprite() = default;

void NumbersSprite::Initialize(const CalyxEngine::Vector2& pos,const CalyxEngine::Vector2& digitSize) {
	origin_    = pos;
	digitSize_ = digitSize;
	SetValue(0);
}

void NumbersSprite::SetPosition(const CalyxEngine::Vector2& pos) {
	origin_      = pos;
	dirtyLayout_ = true;
}

void NumbersSprite::SetDigitSize(const CalyxEngine::Vector2& size) {
	digitSize_   = size;
	dirtyLayout_ = true;
}

void NumbersSprite::SetSpacing(float px) {
	spacing_     = px;
	dirtyLayout_ = true;
}

void NumbersSprite::SetAlign(DigitsAlign a) {
	align_       = a;
	dirtyLayout_ = true;
}

void NumbersSprite::SetAnchor(const CalyxEngine::Vector2& anc) {
	anchor_ = anc;
	for(auto& sp : sprites_) sp->SetAnchorPoint(anchor_);
	dirtyLayout_ = true;
}

void NumbersSprite::SetMinDigits(int n) {
	minDigits_   = (std::max)(1,n);
	dirtyDigits_ = true;
}

void NumbersSprite::SetMaxDigits(int n) {
	maxDigits_   = (std::max)(1,n);
	dirtyDigits_ = true;
}

std::vector<int> NumbersSprite::ToDigits_(int value) {
	if(value <= 0) return {0};
	std::vector<int> ds;
	while(value > 0) {
		ds.push_back(value % 10);
		value /= 10;
	}
	std::reverse(ds.begin(),ds.end());
	return ds;
}

void NumbersSprite::SetValue(int value) {
	// クランプ（上限 10^maxDigits - 1）
	const int maxVal = (maxDigits_ >= 9) ? 999999999 : static_cast<int>(std::pow(10,maxDigits_) - 1);
	if(value < 0) value = 0;
	if(value > maxVal) value = maxVal;

	if(value_ != value) {
		value_       = value;
		dirtyDigits_ = true;
	}
}

void NumbersSprite::RebuildSpritesIfNeeded(size_t needCount) {
	// スプライト数が不足していたら追加、余っていたら保持（再利用）
	if(sprites_.size() < needCount) {
		const size_t old = sprites_.size();
		sprites_.resize(needCount);
		for(size_t i = old; i < needCount; ++i) {
			const std::string tex0 = JoinPath_(dir_,"0" + ext_);

			sprites_[i] = std::make_unique<Sprite>(tex0.c_str());
			sprites_[i]->Initialize(origin_,digitSize_);
			sprites_[i]->SetAnchorPoint(anchor_);
			sprites_[i]->SetIsVisible(true);
		}
	}
	// 余りのスプライトは非表示（再利用想定）
	for(size_t i = needCount; i < sprites_.size(); ++i) { sprites_[i]->SetIsVisible(false); }
}

void NumbersSprite::ApplyTexturesDiffOnly() {
	// 変化のあった桁だけ SetTexture する
	for(size_t i = 0; i < digits_.size(); ++i) {
		const int d = digits_[i];
		if(i < prevDigits_.size() && prevDigits_[i] == d) continue;

		const std::string full = JoinPath_(dir_,std::to_string(d) + ext_);
		sprites_[i]->SetTexture(full.c_str());
	}
	prevDigits_ = digits_;
}

void NumbersSprite::Relayout() {
	const float w      = digitSize_.x;
	const float totalW = static_cast<float>(digits_.size()) * w + (std::max)(0,int(digits_.size()) - 1) * spacing_;

	float startX = origin_.x;
	switch(align_) {
	case DigitsAlign::Left:
		startX = origin_.x;
		break;
	case DigitsAlign::Center:
		startX = origin_.x - totalW * 0.5f;
		break;
	case DigitsAlign::Right:
		startX = origin_.x - totalW;
		break;
	}

	float x = startX;
	for(size_t i = 0; i < digits_.size(); ++i) {
		sprites_[i]->SetPosition({x + digitSize_.x * anchor_.x,origin_.y});
		sprites_[i]->SetIsVisible(true);
		x += w + spacing_;
	}
	dirtyLayout_ = false;
}

void NumbersSprite::Update() {
	if(dirtyDigits_) {
		// 数値→桁列
		digits_ = ToDigits_(value_);
		if((int)digits_.size() < minDigits_) {
			// 先頭ゼロ埋め
			std::vector<int> padded(minDigits_ - digits_.size(),0);
			padded.insert(padded.end(),digits_.begin(),digits_.end());
			digits_.swap(padded);
		}

		RebuildSpritesIfNeeded(digits_.size());
		ApplyTexturesDiffOnly();
		dirtyDigits_ = false;
		dirtyLayout_ = true;
	}

	if(dirtyLayout_) Relayout();

	for(auto& sp : sprites_)
		if(sp->GetIsVisible()) sp->Update();
}

void NumbersSprite::Draw(SpriteRenderer* renderer) const {
	// 表示中のスプライトをすべて描画登録
	for(auto& up : sprites_) { if(up && up->GetIsVisible()) renderer->Register(up.get()); }
}

std::vector<Sprite*> NumbersSprite::GetSpritesRaw() const {
	std::vector<Sprite*> out;
	out.reserve(sprites_.size());
	for(auto& up : sprites_)
		if(up && up->GetIsVisible()) out.push_back(up.get());
	return out;
}