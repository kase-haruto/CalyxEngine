#pragma once
/* ========================================================================
 * 	include space
 * ======================================================================*/
// engine
#include <Engine/Objects/2D/Hud/BaseHud.h>

class ClearLogoHudConfig;
/* ========================================================================
 *	ClearLogoHud
 *	- クリアロゴのHUD
 *	-リザルト画面でクリアロゴを表示するためのHUDクラス
 * ====================================================================== */
class ClearLogoHud final
	: public Calyx2D::BaseHud {
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
	void TopGui()override;
	/**
	 * \brief Gui表示
	 */
	void DerivedGui() override;

private:
	//===================================================================*/
	//		private method
	//===================================================================*/
	/**
	 * \brief データからコンフィグを作成
	 */
	void CreateConfigFromData();

private:
	//===================================================================*/
	//		private members
	//===================================================================*/
	std::unique_ptr<ClearLogoHudConfig> configData_;
};