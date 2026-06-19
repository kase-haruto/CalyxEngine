#pragma once

#include <Data/Engine/Configs/Scene/Objects/Physics/PhysicsBodyConfig.h>

/*-----------------------------------------------------------------------------------------
 * PhysicsBodyType
 * - 物理応答時の動かし方を表す種別
 * - Static は押し戻しを受けず、Kinematic は位置補正だけを受ける
 *---------------------------------------------------------------------------------------*/
enum class PhysicsBodyType {
	Static = 0,
	Kinematic = 1,
};

/*-----------------------------------------------------------------------------------------
 * PhysicsBody
 * - SceneObject に付与する軽量な物理応答設定
 * - Collider は形状と検出、PhysicsBody は押し戻し可否と移動種別を担当する
 *---------------------------------------------------------------------------------------*/
class PhysicsBody {
public:
	/**
	 * \brief デバッグGUIを表示する
	 */
	void ShowGui();

	/**
	 * \brief コンフィグを適用する
	 * \param config 物理応答設定
	 */
	void ApplyConfig(const PhysicsBodyConfig& config);

	/**
	 * \brief コンフィグを抽出する
	 * \return 物理応答設定
	 */
	PhysicsBodyConfig ExtractConfig() const;

	/**
	 * \brief 物理応答が有効かを取得する
	 * \return 有効なら true
	 */
	bool IsEnabled() const { return enabled_; }

	/**
	 * \brief 物理応答の有効状態を設定する
	 * \param enabled 有効にするか
	 */
	void SetEnabled(bool enabled) { enabled_ = enabled; }

	/**
	 * \brief Body種別を取得する
	 * \return Body種別
	 */
	PhysicsBodyType GetBodyType() const { return bodyType_; }

	/**
	 * \brief Body種別を設定する
	 * \param type Body種別
	 */
	void SetBodyType(PhysicsBodyType type) { bodyType_ = type; }

	/**
	 * \brief 押し戻し反映率を取得する
	 * \return 反映率
	 */
	float GetPushbackRatio() const { return pushbackRatio_; }

private:
	bool enabled_ = true;							 //< 物理応答を行うか
	PhysicsBodyType bodyType_ = PhysicsBodyType::Static; //< 押し戻し時の移動種別
	float pushbackRatio_ = 1.0f;						 //< 押し戻し量の反映率
};
