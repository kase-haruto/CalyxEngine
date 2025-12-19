#pragma once

#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/Objects/Transform/Transform.h>

#include <memory>
#include <string>

struct CxMath::Vector2;
struct CxMath::Vector4;
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
	void Initialize(const CxMath::Vector2& position, const CxMath::Vector2& size);
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

	/**
	 * \brief デバッグ用gui表示
	 * \param label 表示名
	 */
	void ShowGui(const std::string& label = "HpGauge");

	//--------- accessor -----------------------------------------------------
	// getter
	Sprite* GetMainGauge() const { return blueGauge_.get(); }
	Sprite* GetDamageGauge() const { return redGauge_.get(); }
	Sprite* GetFrameSprite() const { return frameSprite_.get(); }

	// setter
	void SetAncorPoint(const CxMath::Vector2& point) const;

private:
	//=================================================================
	//				 private method
	//=================================================================
	/**
	 * \brief 各スプライトにtransformを同期させる
	 */
	void SyncTransform() const;

private:
	//=================================================================
	//				 private variable
	//=================================================================
	float maxHp_;     //< 最大hp
	float currentHp_; //< 現在のhp

	float blueRatio_; //< 青ゲージの割合
	float redRatio_;  //< 赤ゲージの割合

	float shakeTimer_   = 0.0f; //< 揺れタイマー
	float visibleTimer_ = 0.0f; //< 可視タイマー

	CxMath::Vector2     baseSize_;  //< 基本サイズ（初期サイズ）
	Transform2D transform_; //< 変換情報（位置・回転・スケール）

	std::unique_ptr<Sprite> blueGauge_;   //< 青ゲージ（現在 HP）
	std::unique_ptr<Sprite> redGauge_;    //< 赤ゲージ（ダメージ遅延）
	std::unique_ptr<Sprite> frameSprite_; //< フレーム
};