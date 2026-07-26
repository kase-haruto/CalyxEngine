#pragma once

/*-----------------------------------------------------------------------------------------
 * PhysicsSystem
 * - コライダー同士の物理応答を解決するシステム
 * - CollisionManager が管理するコライダーを読み取り、押し戻し可能な Body だけを補正する
 *---------------------------------------------------------------------------------------*/
/**
 * @brief PhysicsSystemの機能を提供するクラスです。
 */
class PhysicsSystem {
public:
	/**
	 * \brief インスタンスを取得
	 * \return PhysicsSystem のインスタンス
	 */
	static PhysicsSystem* GetInstance();

	/**
	 * \brief 描画フレーム時間を蓄積して固定物理ステップを実行する
	 * \param deltaTime 描画フレームの経過時間
	 */
	void Update(float deltaTime);

	/**
	 * \brief 1回分の固定物理ステップを実行する
	 * \param fixedDeltaTime 固定時間ステップ
	 */
	void Step(float fixedDeltaTime);

	/**
	 * \brief すべてのコライダーの物理応答を解決する
	 */
	void ResolveAll();

private:
	PhysicsSystem() = default;

	/**
	 * \brief Dynamic Bodyへ重力を適用して位置を積分する
	 * \param fixedDeltaTime 固定時間ステップ
	 */
	void IntegrateDynamicBodies(float fixedDeltaTime);

private:
	float accumulator_ = 0.0f;            //< 未処理の物理シミュレーション時間
	float fixedDeltaTime_ = 1.0f / 60.0f; //< 物理シミュレーションの固定時間幅
	int maxSubSteps_ = 4;                  //< 1描画フレームで許可する最大物理ステップ数
};
