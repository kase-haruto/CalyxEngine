#pragma once
/* ========================================================================
 * 	include space
 * ======================================================================*/
// engine
#include <Engine/Objects/2D/Hud/BaseHud.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

/* ========================================================================
 *	ClearLogoHud
 *	- クリアロゴのHUD
 *	-リザルト画面でクリアロゴを表示するためのHUDクラス
 * ====================================================================== */
class ClearLogoHud final
	: public Calyx2D::BaseHud
	 ,public CalyxEngine::SerializableObject {
public:
	//===================================================================*/
	//		public methods
	//===================================================================*/
	ClearLogoHud();
	~ClearLogoHud() override;

	/**
	 * \brief 初期化 座標指定版
	 * \param pos
	 */
	void Initialize();

	/**
	 * \brief デバッグ用GUI表示
	 */
	void ShowGui();

	//- accessor --------------------------------------------------------//
	// getter
	CalyxEngine::ParamPath GetParamPath() const override;

private:
	//===================================================================*/
	//		private method
	//===================================================================*/
	/**
	 * \brief 設定の構築
	 */
	void CreateConfig();
	/**
	 * \brief シリアライズ可能パラメータの初期化
	 */
	void InitializeSerializableParm();

private:
	//===================================================================*/
	//		private members
	//===================================================================*/
	CalyxMath::Vector2 startPosition_; //< 開始位置
	CalyxMath::Vector2 stayPosition_;  //< 滞在位置
	CalyxMath::Vector2 scale_;         //< スケール

	float duration_ = 0.5f; //< モーション時間
};