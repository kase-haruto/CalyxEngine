#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include "Data/Game/Config/Hud/EnemyResultConfig.h"
#include "Game/Scene/Transition/ResultTransitionPayload.h"

#include <Engine/Objects/2D/Hud/BaseHud.h>
#include <Engine/Objects/2D/NumbersSprite/NumbersSprite.h>

/*-----------------------------------------------------------------------------------------
 * EnemyResultHud
 * - 敵撃破数の行表示 + カウントアップ演出 + 行ディレイ
 * - ScoreResultHud と同じ思想（BaseHud + StateTimer + Config + GUI）
 *---------------------------------------------------------------------------------------*/
class EnemyResultHud final
	: public Calyx2D::BaseHud {
private:
	//===================================================================*/
	//			structs
	//===================================================================*/
	/*---------------------------------------------------------------------
	 * 敵撃破行データ構造体
	 * - 敵アイコンと撃破数表示をまとめた構造体
	 *--------------------------------------------------------------------*/
	struct EnemyRow {
		std::unique_ptr<Calyx2D::SpriteObject2d> icon;
		std::unique_ptr<NumbersSprite>           numbers;

		uint32_t finalCount   = 0;
		float    currentCount = 0.0f;

		CalyxUtil::StateTimer delayTimer;
		CalyxUtil::StateTimer countTimer;

		float rowY = 0.0f; // 行ローカルY
	};

public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	/* コンストラクタ / デストラクタ */
	EnemyResultHud();
	~EnemyResultHud() override;

	/**
	 * \brief 初期化処理
	 * \param payload 撃破情報
	 */
	void Initialize(const ResultTransitionPayload& payload);
	/**
	 * \brief 更新処理
	 * \param dt  デルタタイム
	 */
	void Update(float dt) override;
	/**
	 * \brief スプライト描画
	 * \param renderer レンダラー
	 */
	void Draw(SpriteRenderer* renderer) const override;

private:
	/**
	 * \brief 滞在フェーズ更新
	 * \param dt デルタタイム
	 */
	void StayUpdate(float dt) override;
	/**
	 * \brief デバッグ用GUI表示
	 */
	void TopGui() override;
	/**
	 * \brief Gui表示
	 */
	void DerivedGui() override;
	/**
	 * \brief データからコンフィグ作成
	 */
	void CreateMotionFromConfig();
	/**
	 * \brief コンフィグからモーション再構築
	 */
	void RebuildMotionFromConfig();

private:
	//===================================================================*/
	//			private members
	//===================================================================*/
	std::unique_ptr<EnemyResultConfig> configData_ = nullptr;
	std::vector<EnemyRow> rows_;
};