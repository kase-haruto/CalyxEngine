#pragma once
/*=========================================================================== 
 *	include space
 * ========================================================================*/
// c++
#include <memory>
#include <vector>
// engine
#include <Engine/Objects/2D/Object2d/SpriteObject2d.h>
#include <Engine/Objects/2D/NumbersSprite/NumbersSprite.h>
// game
#include "Game/2d/Hud/EnemyRowHud.h"
#include "Game/2d/Hud/ScoreResultHud.h"

#include <Game/Scene/Transition/ResultTransitionPayload.h>
#include<Game/2d/Hud/ClearLogoHud.h>

/*-----------------------------------------------------------------------------------------
 * ResultOverlay class
 * - 結果画面オーバーレイクラス
 * - スコアや撃破数の表示を担当
 *---------------------------------------------------------------------------------------*/
class ResultOverlay {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	/** \brief コンストラクタ / デストラクタ*/
	ResultOverlay();
	virtual ~ResultOverlay();
	/**
	 * \brief 初期化処理
	 */
	void Initialize(const ResultTransitionPayload& payload);
	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt);
	/**
	 * \brief スプライト描画
	 * \param renderer レンダラー
	 */
	void Draw(class SpriteRenderer* renderer) const;
	/*
	 * \brief デバッグGUI表示
	 */
	void ShowGUi();

private:
	//===================================================================*/
	//			private members
	//===================================================================*/
	std::unique_ptr<ClearLogoHud>            clearLogo_;    //< クリアロゴ
	std::unique_ptr<Calyx2D::SpriteObject2d> continueIcon_; //< コンティニューアイコン
	std::unique_ptr<ScoreResultHud>          scoreHud_;     //< スコアHUD
	std::unique_ptr<EnemyResultHud>          enemyHud_;     //< 敵撃破数HUD

	float timer_        = 0.0f;
	bool  showContinue_ = false;
};