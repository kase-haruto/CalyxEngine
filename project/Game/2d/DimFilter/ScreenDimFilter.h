#pragma once
#include "Data/Game/Config/DimFilter/DimFilterConfig.h"
#include "Engine/Foundation/Clock/StateTimer.h"
#include "Engine/Objects/2D/Object2d/SpriteObject2d.h"
/*===========================================================================
 *	include space
 * ========================================================================*/

/*-----------------------------------------------------------------------------------------
 * ScreenDimFilter class
 * - 画面全体を暗くするフィルタークラス
 *---------------------------------------------------------------------------------------*/
class ScreenDimFilter {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	/** \brief コンストラクタ / デストラクタ*/
	ScreenDimFilter();
	virtual ~ScreenDimFilter();
	/**
	 * \brief 初期化
	 * \param screenSize 画面サイズ（kGameSize）
	 */
	void Initialize(const CalyxMath::Vector2& screenSize);
	/**
	 * \brief 更新
	 */
	void Update(float dt);
	/**
	 * \brief 描画
	 */
	void Draw(SpriteRenderer* renderer) const;
	/**
	 * \brief フェードイン開始
	 * \param duration    フェード時間
	 */
	void StartFadeIn();
	/**
	 * \brief フェードアウト開始
	 * \param duration フェード時間
	 */
	void StartFadeOut();
	/**
	 * \brief 即時表示
	 */
	void SetAlpha(float alpha);
	bool IsFinished() const { return isFinished_; }
	float GetAlpha() const { return currentAlpha_; }

protected:
	std::unique_ptr<Calyx2D::SpriteObject2d> sprite_;
	std::unique_ptr<DimFilterConfig> configData_ = nullptr;
	CalyxUtil::StateTimer fadeTimer_;
	float currentAlpha_ = 0.0f;
	bool isFading_ = false;
	bool isFinished_ = false;
};