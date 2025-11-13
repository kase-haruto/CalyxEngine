#pragma once

#include <Engine/Renderer/Sprite/Sprite.h>
#include <memory>

struct Vector2;
struct Vector4;
class Sprite;

/************************************
 * \brief
 * hpゲージを表示するためのクラス
 ************************************/
class HpGauge {
public:
	//=================================================================
	//				 public method
	//=================================================================
	HpGauge(float maxHp);
	~HpGauge();

	/**
	 * \brief 初期化処理
	 * \param position	座標
	 * \param size		サイズ
	 */
	void Initialize(const Vector2& position, const Vector2& size);
	/**
	 * \brief 更新処理
	 * \param dt 時間
	 */
	void Update(float dt);
	/**
	 * \brief 参照するHPをセット
	 * \param hp 参照hp
	 */
	void SetHp(float hp);

	//--------- accessor -----------------------------------------------------
	Sprite* GetMainGauge() const { return blueGauge_.get(); }
	Sprite* GetDamageGauge() const { return redGauge_.get(); }
	Sprite* GetFrameSprite() const { return frameSprite_.get(); }

private:
	//=================================================================
	//				 private method
	//=================================================================
	/**
	 * \brief 表示する色の計算
	 * \param ratio hp割合
	 * \return 計算結果の色を返す
	 */
	Vector4 ComputeColor(float ratio) const;

private:
	//=================================================================
	//				 private variable
	//=================================================================
	float maxHp_;		//< 最大hp
	float currentHp_;	//< 現在のhp

	float blueRatio_;	//< 青ゲージの割合
	float redRatio_;	//< 赤ゲージの割合

	float shakeTimer_   = 0.0f;	//< 揺れタイマー
	float visibleTimer_ = 0.0f;	//< 可視タイマー

	Vector2 baseSize_;			//< 基本サイズ

	std::unique_ptr<Sprite> blueGauge_;		//< 青ゲージ
	std::unique_ptr<Sprite> redGauge_;		//< 赤ゲージ
	std::unique_ptr<Sprite> frameSprite_;	//< フレーム
};