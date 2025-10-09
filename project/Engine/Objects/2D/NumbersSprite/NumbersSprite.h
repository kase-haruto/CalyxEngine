#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */

#include <Engine/Foundation/Math/Vector2.h>

// stl
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Sprite;

/* ========================================================================
/*		スプライトを使用して数字を表示
/* ===================================================================== */
class NumbersSprite {
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

	void Update();
	void ShowGui(const std::string& label = "NumbersSprite");

	//--------- accessor -------------------------------------------------//
	// getter
	const Vector2& GetAnchor() const;
	const Vector2& GetSpriteSize() const;
	const float	   GetValue() const;
	const float	   GetSpace() const;

	// setter
	void SetAnchor(const Vector2& anc);
	void SetSpriteSize(const Vector2& size);
	void SetValue(int val);
	void SetMinDigit(int digit);
	void SetMaxDigit(int digit);
	void SetSpace(float space);

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/

	// ファイルパス
	std::string dir_ = "Resources/Assets/Textures/Numbers/";
	std::string ext_ = ".png";

	std::vector<std::unique_ptr<Sprite>> sprites_;	 //< 表示する数字の配列
	std::vector<unsigned int>			 digits_;	 //< 表示したい数字の各桁
	std::vector<unsigned int>			 preDigits_; //< 差分用

	Vector2 anchor_{0.5f, 0.5f};	   //< spriteのアンカー
	Vector2 spriteSize_{64.0f, 64.0f}; //< spriteのサイズ

	int	  value_;	  //< 表示する数字
	int	  minDigits_; //< 最少桁数
	int	  maxDigits_; //< 最大桁数
	float space_;	  //< 表示間隔
};
