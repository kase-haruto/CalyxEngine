#pragma once
/*====================================================================
 *		include space
 *====================================================================*/
// engine
#include "Engine/Objects/2D/NumbersSprite/NumbersSprite.h"
#include <Data/Game/Config/Hud/ScoreResultConfig.h>
#include <Engine/Objects/2D/Hud/BaseHud.h>

/*-----------------------------------------------------------------------------------------
 *	ScoreResultHud class
 *	- スコアリザルトHUDクラス
 *	- リザルト画面でスコア表示を担当するHUDクラス
 *---------------------------------------------------------------------------------------*/
class ScoreResultHud final
	: public Calyx2D::BaseHud {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	/* コンストラクタ / デストラクタ */
	ScoreResultHud();
	~ScoreResultHud() override;
	/**
	 * \brief 初期化
	 */
	void Initialize();
	/**
	 * \brief 更新
	 */
	void Update(float dt)override;
	/**
	 * \brief 描画
	 */
	void Draw(SpriteRenderer* renderer) const override;

	//- accessor ---------------------------------------------------------//
	void SetScore(uint32_t score);

private:
	/**
	 * \brief 滞在フェーズ更新
	 */
	void StayUpdate(float dt)override;
	/**
	 * \brief デバッグ用GUI表示
	 */
	void TopGui()override;
	/**
	 * \brief Gui表示
	 */
	void DerivedGui()override;
	void CreateMotionFromConfig();
	void RebuildMotionFromConfig();
	/**
	 * \brief データからコンフィグ作成
	 */
	void CreateConfigFromData();

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	std::unique_ptr<NumbersSprite> numbers_ = nullptr;

	CalyxUtil::StateTimer delayTimer_;
	CalyxUtil::StateTimer countTimer_;

	uint32_t finalScore_   = 0;
	float    currentScore_ = 0.0f;

	std::unique_ptr<ScoreResultConfig> configData_ = nullptr;

	bool started_ = false;
	bool finished_ = false;
};