#pragma once
/// ===================================================================== */
///  include space
/// ===================================================================== */
// engine
#include <Engine/Foundation/Utility/Animation/SimpleAnimation.h>
// game
#include "StageGimmickActor.h"

/*-----------------------------------------------------------------------------------------
 * FalldownGimmickActor
 * - 落下ギミックのクラス
 * - 落下ギミックの具体的な動作やアニメーションを実装するクラス
 *---------------------------------------------------------------------------------------*/
class FalldownGimmickActor final
	: public StageGimmickActor {
public:
	//====================================================================*/
	//			public methods
	//====================================================================*/
	FalldownGimmickActor();
	~FalldownGimmickActor() override;

	/**
	 * デバッグ用gui表示
	 */
	void DerivativeGui() override;

protected:
	/**
	 * \brief 未動作時の処理
	 * \param dt
	 */
	virtual void IdleUpdate(float dt);
	/**
	 * \brief トリガーされた瞬間の処理
	 * \param dt
	 */
	virtual void OnTriggered();
	/**
	 * \brief 動作中の処理
	 * \param dt
	 */
	virtual void RunningUpdate(float dt);
	/**
	 * \brief 動作終了時の処理
	 */
	virtual void OnFinished();

private:
	//====================================================================*/
	//			private methods
	//====================================================================*/
	CalyxUtil::SimpleAnimation<CalyxMath::Vector3> falldownAnimation_; //< 落下アニメーション
};