#include "NumbersSprite.h"

// engine
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

// std
#include <algorithm>
#include <cmath>

static inline std::string Join2_(const std::string& a,const std::string& b) {
#ifdef _WIN32
	const char sep = '\\';
#else
	const char sep='/';
#endif
	if(a.empty()) return b;
	if(a.back() == '/' || a.back() == '\\') return a + b;
	return a + sep + b;
}

std::string NumbersSprite::JoinPath_(const std::string& a,const std::string& b) { return Join2_(a,b); }


/* ======================= Ctor / Dtor ======================= */
NumbersSprite::NumbersSprite() = default;
NumbersSprite::NumbersSprite(int value) : value_(value) {}
NumbersSprite::~NumbersSprite() = default;

/* ======================= Initialize ======================== */
void NumbersSprite::Initialize(const Vector2& pos,const Vector2& digitSize) {
	origin_     = pos;
	spriteSize_ = digitSize;

	// 最初の値を反映
	dirtyDigits_ = true;
	dirtyLayout_ = true;
}

/* ======================== Update =========================== */
void NumbersSprite::Update() {
	// 桁更新（数値→桁列→スプライト反映）
	if(dirtyDigits_) {
		// 上限を計算してクランプ
		const int maxVal = (maxDigits_ >= 9)
			                   ? 999999999
			                   : static_cast<int>(std::pow(10,maxDigits_) - 1);

		int v = value_;
		if(v < 0) v = 0;
		if(v > maxVal) v = maxVal;

		digits_ = ToDigits_(v);

		// 先頭ゼロ埋め
		if(static_cast<int>(digits_.size()) < minDigits_) {
			const int             need = minDigits_ - static_cast<int>(digits_.size());
			std::vector<unsigned> padded;
			padded.reserve(minDigits_);
			padded.insert(padded.end(),static_cast<size_t>(need),0u);
			padded.insert(padded.end(),digits_.begin(),digits_.end());
			digits_.swap(padded);
		}

		RebuildSpritesIfNeeded_(digits_.size());
		ApplyTexturesDiffOnly_();

		dirtyDigits_ = false;
		dirtyLayout_ = true; // 桁数が変わったらレイアウトも必要
	}

	if(dirtyLayout_) {
		Relayout_();
		dirtyLayout_ = false;
	}

	// スプライトの更新
	for(auto& sp : sprites_) { if(sp && sp->GetIsVisible()) sp->Update(); }
}

/* ======================== GUI ============================== */
void NumbersSprite::ShowGui(const std::string& label) { (void)label; }

/* ===================== private helpers ===================== */
std::vector<unsigned> NumbersSprite::ToDigits_(int value) {
	if(value <= 0) return {0u};
	std::vector<unsigned> ds;
	while(value > 0) {
		ds.push_back(static_cast<unsigned>(value % 10));
		value /= 10;
	}
	std::reverse(ds.begin(),ds.end());
	return ds;
}

void NumbersSprite::RebuildSpritesIfNeeded_(size_t needCount) {
	if(sprites_.size() < needCount) {
		const size_t old = sprites_.size();
		sprites_.resize(needCount);
		for(size_t i = old; i < needCount; ++i) {
			const std::string path0 = JoinPath_(dir_,"0" + ext_);
			sprites_[i]             = std::make_unique<Sprite>(path0.c_str());
			sprites_[i]->Initialize(origin_,spriteSize_);
			sprites_[i]->SetAnchorPoint(anchor_);
			sprites_[i]->SetIsVisible(true);
		}
	}
	// 余剰は非表示（再利用前提）
	for(size_t i = needCount; i < sprites_.size(); ++i) { if(sprites_[i]) sprites_[i]->SetIsVisible(false); }
}

void NumbersSprite::ApplyTexturesDiffOnly_() {
	// 差分のあった桁だけ SetTexture を呼ぶ
	for(size_t i = 0; i < digits_.size(); ++i) {
		const unsigned d = digits_[i];
		if(i < preDigits_.size() && preDigits_[i] == d) continue;

		const std::string full = JoinPath_(dir_,std::to_string(d) + ext_);
		//sprites_[i]->SetTexture(full.c_str());
		sprites_[i]->SetIsVisible(true);
	}
	preDigits_ = digits_;
}

void NumbersSprite::Relayout_() {
	const float w      = spriteSize_.x;
	const float totalW = static_cast<float>(digits_.size()) * w
	                     + (std::max)(0,static_cast<int>(digits_.size()) - 1) * space_;

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
		sprites_[i]->SetPosition({x + spriteSize_.x * anchor_.x,origin_.y});
		x += w + space_;
	}
}


/* ======================== setters ========================== */
void NumbersSprite::SetAnchor(const Vector2& anc) {
	anchor_ = anc;
	for(auto& sp : sprites_) if(sp) sp->SetAnchorPoint(anchor_);
	dirtyLayout_ = true;
}

void NumbersSprite::SetSpriteSize(const Vector2& size) {
	spriteSize_  = size;
	dirtyLayout_ = true;
}

void NumbersSprite::SetPosition(const Vector2& pos) {
	origin_      = pos;
	dirtyLayout_ = true;
}

void NumbersSprite::SetValue(int val) {
	if(value_ != val) {
		value_       = val;
		dirtyDigits_ = true;
	}
}

void NumbersSprite::SetMinDigit(int digit) {
	if(digit < 1) digit = 1;
	if(minDigits_ != digit) {
		minDigits_   = digit;
		dirtyDigits_ = true;
	}
}

void NumbersSprite::SetMaxDigit(int digit) {
	if(digit < 1) digit = 1;
	if(maxDigits_ != digit) {
		maxDigits_   = digit;
		dirtyDigits_ = true;
	}
}

void NumbersSprite::SetSpace(float space) {
	space_       = space;
	dirtyLayout_ = true;
}

void NumbersSprite::SetAlign(DigitsAlign a) {
	align_       = a;
	dirtyLayout_ = true;
}

void NumbersSprite::SetTextureDir(const std::string& dir) {
	dir_         = dir;
	dirtyDigits_ = true; // パスが変わる＝再貼替
}

void NumbersSprite::SetTextureExt(const std::string& ext) {
	ext_         = ext;
	dirtyDigits_ = true;
}


/* ======================== getters ========================== */
const Vector2&             NumbersSprite::GetAnchor() const { return anchor_; }
const Vector2&             NumbersSprite::GetSpriteSize() const { return spriteSize_; }
const Vector2&             NumbersSprite::GetPosition() const { return origin_; }
int                        NumbersSprite::GetValue() const { return value_; }
float                      NumbersSprite::GetSpace() const { return space_; }
int                        NumbersSprite::GetMinDigits() const { return minDigits_; }
int                        NumbersSprite::GetMaxDigits() const { return maxDigits_; }
NumbersSprite::DigitsAlign NumbersSprite::GetAlign() const { return align_; }

std::vector<Sprite*> NumbersSprite::GetSpritesRaw() const {
	std::vector<Sprite*> out;
	out.reserve(sprites_.size());
	for(auto& sprite : sprites_) { if(sprite && sprite->GetIsVisible()) out.push_back(sprite.get()); }
	return out;
}