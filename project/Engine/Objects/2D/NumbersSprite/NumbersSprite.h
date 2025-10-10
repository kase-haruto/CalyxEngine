#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */

#include <Engine/Foundation/Math/Vector2.h>

// stl
#include <memory>
#include <string>
#include <vector>

class Sprite;

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

public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	NumbersSprite();
	NumbersSprite(int value);
	~NumbersSprite();

	void Initialize(const Vector2& pos,const Vector2& digitSize);
	void Update();
	void ShowGui(const std::string& label = "NumbersSprite");

	//--------- accessor -------------------------------------------------//
	// getter
	DigitsAlign    GetAlign() const;
	const Vector2& GetAnchor() const;
	const Vector2& GetSpriteSize() const;
	const Vector2& GetPosition() const;

	float GetSpace() const;
	int   GetValue() const;
	int   GetMinDigits() const;
	int   GetMaxDigits() const;


	// 描画登録用（可視の桁のみ返す）
	std::vector<Sprite*> GetSpritesRaw() const;

	// setter
	void SetAnchor(const Vector2& anc);
	void SetSpriteSize(const Vector2& size); // 1桁サイズ
	void SetPosition(const Vector2& pos);    // 基準位置
	void SetValue(int val);                  // 数値（クランプあり）
	void SetMinDigit(int digit);             // 先頭ゼロ埋め桁
	void SetMaxDigit(int digit);             // 表示上限桁
	void SetSpace(float space);              // 桁間隔(px)
	void SetAlign(DigitsAlign a);            // 左/中央/右

	// 数字テクスチャの格納場所（dir/0.png〜9.png）
	void SetTextureDir(const std::string& dir);
	void SetTextureExt(const std::string& ext);

private:
	//===================================================================*/
	//                  private methods
	//===================================================================*/
	static std::vector<unsigned> ToDigits_(int value);

	void RebuildSpritesIfNeeded_(size_t needCount);
	void ApplyTexturesDiffOnly_();
	void Relayout_();

	static std::string JoinPath_(const std::string& a,const std::string& b);

private:
	// ファイルパス
	std::string dir_ = "Resources/Assets/Textures/Numbers";
	std::string ext_ = ".png";

	// 表示要素
	std::vector<std::unique_ptr<Sprite>> sprites_;   //< 各桁スプライト
	std::vector<unsigned>                digits_;    //< 今フレ表示桁
	std::vector<unsigned>                preDigits_; //< 前フレ桁(差分用)

	// レイアウト
	Vector2     anchor_{0.5f,0.5f};       //< 各桁アンカー
	Vector2     spriteSize_{64.0f,64.0f}; //< 1桁サイズ
	Vector2     origin_{0.0f,0.0f};       //< 基準位置
	float       space_ = 2.0f;            //< 桁間隔
	DigitsAlign align_ = DigitsAlign::Right;

	// 値とフォーマット
	int value_     = 0; //< 表示値
	int minDigits_ = 1; //< 最少桁（ゼロ埋め）
	int maxDigits_ = 5; //< 最大桁（10^maxDigits - 1 にクランプ）

	// ダーティフラグ
	bool dirtyDigits_ = true;
	bool dirtyLayout_ = true;
};