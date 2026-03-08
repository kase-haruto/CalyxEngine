#pragma once
/// ===================================================================== */
///  include space
/// ===================================================================== */
// engine
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Foundation/Utility/Animation/SimpleAnimation.h>
#include <Engine/Foundation/Utility/Animation/Transform/TransformAnimation.h>

// game
#include "StageGimmickActor.h"

/*-----------------------------------------------------------------------------------------
 * FalldownGimmickActor
 * - 倒れこみギミックのクラス
 * - 背景アクタが倒れこんでくるギミックを表現するクラス
 * - 倒れこみのアニメーションとエフェクトを管理する
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

	/**
	 * \brief JSONから派生クラスの設定を適用
	 */
	void ApplyDerivedConfigFromJson(const nlohmann::json& root, const nlohmann::json* derived) override;

	/**
	 * \brief 派生クラスの設定をJSONに抽出
	 */
	void ExtractDerivedConfigToJson(nlohmann::json& root, nlohmann::json& derived) const override;

protected:
	void IdleUpdate(float dt) override;
	/**
	 * \brief トリガーされた瞬間の処理
	 * \param dt
	 */
	void OnTriggered() override;
	/**
	 * \brief 動作中の処理
	 * \param dt
	 */
	void RunningUpdate(float dt) override;
	/**
	 * \brief 動作終了時の処理
	 */
	void OnFinished() override;

private:
	//====================================================================*/
	//			private methods
	//====================================================================*/
	std::weak_ptr<CalyxEffect::FxObject>			 falldownFx_{};		  //< 倒れている最中のエフェクト
	std::unique_ptr<CalyxEngine::TransformAnimation> transformAnimation_; //< 倒れこみアニメーション

	CalyxMath::Quaternion animationStartRotation_;														 //< アニメーション開始時の回転
	CalyxMath::Quaternion animationEndRotation_;														 //< アニメーション終了時
	float				  animationDuration_ = 1.0f;													 //< アニメーション時間
	float				  stopProgress_		= 0.0f;													 //< 停止進捗
	int32_t				  animationEaseType_ = static_cast<int32_t>(CalyxEase::EaseType::EaseInOutSine); //< アニメーションのイージングタイプ
	
};