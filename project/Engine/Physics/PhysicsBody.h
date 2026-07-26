#pragma once

#include <Data/Engine/Configs/Scene/Objects/Physics/PhysicsBodyConfig.h>
#include <Engine/Foundation/Math/Vector3.h>

/*-----------------------------------------------------------------------------------------
 * PhysicsBodyType
 * - 物理応答時の動かし方を表す種別
 * - Staticは固定、Kinematicはゲーム制御、Dynamicは物理シミュレーションで移動する
 *---------------------------------------------------------------------------------------*/
enum class PhysicsBodyType {
	Static = 0,
	Kinematic = 1,
	Dynamic = 2,
};

/*-----------------------------------------------------------------------------------------
 * PhysicsBody
 * - SceneObject に付与する軽量な物理応答設定
 * - Collider は形状と検出、PhysicsBody は押し戻し可否と移動種別を担当する
 *---------------------------------------------------------------------------------------*/
/**
 * @brief PhysicsBodyの機能を提供するクラスです。
 */
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
	void SetBodyType(PhysicsBodyType type);

	/**
	 * \brief 押し戻し反映率を取得する
	 * \return 反映率
	 */
	float GetPushbackRatio() const { return pushbackRatio_; }

	/**
	 * \brief 現在の線形速度を取得する
	 * \return ワールド空間の線形速度
	 */
	const CalyxEngine::Vector3& GetLinearVelocity() const { return linearVelocity_; }

	/**
	 * \brief 線形速度を設定する
	 * \param velocity ワールド空間の線形速度
	 */
	void SetLinearVelocity(const CalyxEngine::Vector3& velocity) { linearVelocity_ = velocity; }

	/**
	 * \brief 線形速度へ変化量を加算する
	 * \param deltaVelocity 加算する速度変化量
	 */
	void AddLinearVelocity(const CalyxEngine::Vector3& deltaVelocity) { linearVelocity_ += deltaVelocity; }

	/**
	 * \brief 重力を線形速度へ反映する
	 * \param gravity ワールド空間の重力加速度
	 * \param fixedDeltaTime 固定時間ステップ
	 */
	void IntegrateForces(const CalyxEngine::Vector3& gravity, float fixedDeltaTime);

	/**
	 * \brief 質量を設定して逆質量を更新する
	 * \param mass 新しい質量
	 */
	void SetMass(float mass);

	/**
	 * \brief 逆質量を取得する
	 * \return Dynamicなら質量の逆数、それ以外なら0
	 */
	float GetInverseMass() const;

private:
	bool enabled_ = true;							 //< 物理応答を行うか
	PhysicsBodyType bodyType_ = PhysicsBodyType::Static; //< 押し戻し時の移動種別
	float pushbackRatio_ = 1.0f;						 //< 押し戻し量の反映率
	CalyxEngine::Vector3 linearVelocity_{};         //< Dynamicのワールド空間線形速度
	bool useGravity_ = true;                        //< Dynamicへ重力を適用するか
	float gravityScale_ = 1.0f;                    //< ワールド重力へ掛ける倍率
	float mass_ = 1.0f;                            //< Dynamicの質量
	float inverseMass_ = 1.0f;                     //< Solverで使用する質量の逆数
};
