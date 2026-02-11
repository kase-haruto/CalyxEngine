#pragma once

/* ========================================================================
/*		include space
/* ===================================================================== */

#include <Engine/Foundation/Math/Vector2.h>

// stl
#include "Engine/Renderer/Sprite/SpriteRenderer.h"

#include <memory>
#include <string>
#include <vector>
#include <array>

#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Renderer/Sprite/Sprite.h>

/* ========================================================================
/*		スプライトを使用して数字を表示
/* ===================================================================== */
class NumbersSprite {
public:
	// 左詰め中央右揃え
	enum class DigitsAlign {
		Left,
		Center,
		Right
	};
	NumbersSprite(std::string dir, std::string ext = ".dds");
	NumbersSprite();

	// 初期化（基準位置・1桁サイズ）
	void Initialize(const CalyxMath::Vector2& pos, const CalyxMath::Vector2& digitSize);

	// 値設定（負・超過値はクランプ）
	void SetValue(int value);


	// レイアウト設定
	void SetPosition(const CalyxMath::Vector2& pos);
	void SetDigitSize(const CalyxMath::Vector2& size);
	void SetSpacing(float px);
	void SetAlign(DigitsAlign a);
	void SetAnchor(const CalyxMath::Vector2& anc);

	// 表示幅の制御
	void SetMinDigits(int n); // 先頭ゼロ埋め（n以下ならゼロで埋める）
	void SetMaxDigits(int n); // 表示の上限桁（n超はクランプ 10^n-1）

	// 更新 & 描画登録
	void				 Update();
	void Draw(SpriteRenderer* renderer) const;
	std::vector<Sprite*> GetSpritesRaw() const;

private:
	static std::vector<int> ToDigits_(int value);

	void RebuildSpritesIfNeeded(size_t needCount); // 必要数に合わせて増減
	void ApplyTexturesDiffOnly();				   // 変化桁だけ差し替え
	void Relayout();

	static std::string JoinPath_(const std::string& a, const std::string& b);

private:
	// 設定
	std::string dir_;
	std::string ext_;
	CalyxMath::Vector2		origin_{0, 0};
	CalyxMath::Vector2		digitSize_{32, 48};
	CalyxMath::Vector2		anchor_{0.5f, 0.5f};
	float		spacing_   = 2.0f;
	DigitsAlign align_	   = DigitsAlign::Right;
	int			minDigits_ = 1; // 先頭ゼロ埋め最小桁
	int			maxDigits_ = 8; // 表示上限（例：8桁= 99,999,999 まで）

	// 状態
	int									 value_ = 0;
	std::vector<int>					 digits_;	  // 例: 120 -> {1,2,0}（ゼロ埋め後）
	std::vector<int>					 prevDigits_; // 差分検出用
	std::vector<std::unique_ptr<Sprite>> sprites_;

	bool dirtyDigits_ = true;
	bool dirtyLayout_ = true;
};