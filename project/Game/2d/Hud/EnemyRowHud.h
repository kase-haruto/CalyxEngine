#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/Objects/2D/Hud/BaseHud.h>
#include <Engine/Objects/2D/NumbersSprite/NumbersSprite.h>

/*----------------------------------------------------------------------*/
/*	敵行表示HUD
 *	- リザルト画面で使用する
/*----------------------------------------------------------------------*/
class EnemyRowHud final
	: public Calyx2D::BaseHud {
public:
	//===================================================================*/
	//			public method
	//===================================================================*/
	EnemyRowHud();
	~EnemyRowHud() override;

	/**
	 * \brief 登場後停止時更新
	 * \param dt
	 */
	void StayUpdate(float dt) override;

private:
	//===================================================================*/
	//			private method
	//===================================================================*/
	NumbersSprite count_;
	int           targetCount_;
};