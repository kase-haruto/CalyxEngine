#pragma once

/*-----------------------------------------------------------------------------------------
 * PhysicsSystem
 * - コライダー同士の物理応答を解決するシステム
 * - CollisionManager が管理するコライダーを読み取り、押し戻し可能な Body だけを補正する
 *---------------------------------------------------------------------------------------*/
class PhysicsSystem {
public:
	/**
	 * \brief インスタンスを取得
	 * \return PhysicsSystem のインスタンス
	 */
	static PhysicsSystem* GetInstance();

	/**
	 * \brief すべてのコライダーの物理応答を解決する
	 */
	void ResolveAll();

private:
	PhysicsSystem() = default;
};
