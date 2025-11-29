#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */

// c++
#include "Game/3dObject/Actor/Boss/Details/BossAttackType.h"

#include <optional>

// fwd
class Boss;

/**
 * \brief ボスの行動を制御するAIクラス
 */
class BossAI {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	BossAI(Boss* owner);
	~BossAI();

	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt);
	/**
	 * \brief 攻撃方法を決定する
	 * \param dt デルタタイム
	 * \return 攻撃方法（攻撃しない場合は std::nullopt）
	 */
	std::optional<BossAttackType>DecideAttack(float dt);
private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	Boss* owner_;
	float cooldownTimer_ = 0.0f;
};