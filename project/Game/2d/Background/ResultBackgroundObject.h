#pragma once
#include "Data/Game/Config/Background/ResultBackgroundObjectConfig.h"
#include "Engine/Objects/2D/Hud/BaseHud.h"
/*===========================================================================
 *	include space
 * ========================================================================*/

/*-----------------------------------------------------------------------------------------
 * ResultBackgroundObject class
 * - リザルト背景オブジェクトクラス
 *---------------------------------------------------------------------------------------*/
class ResultBackgroundObject {
	private:

public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	//** \brief コンストラクタ / デストラクタ*/
	ResultBackgroundObject();
	virtual ~ResultBackgroundObject();
	/**
	 * \brief 初期化処理
	 */
	void Initialize(const std::string& filePath);
	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt);
	/**
	 * \brief スプライト描画
	 * \param renderer レンダラー
	 */
	void Draw(SpriteRenderer* renderer) const;
	void ShowGui();
private:
	//===================================================================*/
	//			private members
	//===================================================================*/
	std::array<std::unique_ptr<Calyx2D::SpriteObject2d>, 2> backgroundSprites_;
	std::array<std::unique_ptr<CalyxUtil::SimpleAnimation<CalyxMath::Vector2>>, 2> moveAnims_;
	std::unique_ptr<ResultBackgroundObjectConfig> configData_;
};