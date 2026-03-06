#pragma once
/// ===================================================================== */
///  include space
/// ===================================================================== */
// engine
#include <Engine/Foundation/Utility/Animation/SimpleAnimation.h>
#include <Engine/Application/Effects/FxObject.h>
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
	 * \brief 初期化処理
	 */
	void Initialize() override;

	/**
	 * デバッグ用gui表示
	 */
	void DerivativeGui() override;

protected:
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
	CalyxUtil::SimpleAnimation<float>    falldownAnimation_; //< 落下アニメーション
	std::weak_ptr<CalyxEffect::FxObject> falldownFx_{};      //< 倒れている最中のエフェクト

};