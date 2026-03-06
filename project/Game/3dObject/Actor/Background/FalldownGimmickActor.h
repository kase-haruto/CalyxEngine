#pragma once
/// ===================================================================== */
///  include space
/// ===================================================================== */
// engine
#include <Engine/Application/Effects/FxObject.h>
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
	FalldownGimmickActor(const std::string& modelName, std::optional<std::string> objectName = std::nullopt);
	~FalldownGimmickActor() override;

	/**
	 * \brief 初期化処理
	 */
	void Initialize() override;

	/**
	 * デバッグ用gui表示
	 */
	void DerivativeGui() override;

	/**
	 * \brief タイプ名を取得
	 * \return タイプ名
	 */
	std::string_view GetTypeName() const override { return "FalldownGimmickActor"; }

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
	CalyxUtil::SimpleAnimation<CalyxMath::Quaternion> falldownAnimation_; //< 落下アニメーション
	std::weak_ptr<CalyxEffect::FxObject>			  falldownFx_{};	  //< 倒れている最中のエフェクト
};