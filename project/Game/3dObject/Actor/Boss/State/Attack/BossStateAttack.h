#pragma once
#include "../Base/BaseBossState.h"
#include "Game/3dObject/Actor/Boss/Attack/IBossAttack.h"
#include "Game/3dObject/Actor/Boss/Details/BossAnimType.h"
#include "Game/3dObject/Actor/Boss/Details/BossAttackType.h"

#include <memory>
#include <unordered_map>

/**
 * \brief ボスの攻撃状態クラス
 */
class BossStateAttack final
	: public BaseBossState {
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	BossStateAttack();
	~BossStateAttack() override;

	/**
	 * @brief 更新処理
	 * @param dt
	 */
	void Update(float dt) override;
	/**
	 * @brief 状態に入るときの処理
	 */
	void Enter() override;
	/**
	 * @brief GUI表示
	 */
	void ShowGui() override;

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	/**
	 * \brief 攻撃実行
	 */
	void ExecuteAttack() const;

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	BossAttackType													 attackType_ = BossAttackType::NormalShoot; //< 攻撃タイプ
	std::unordered_map<BossAttackType, std::unique_ptr<IBossAttack>> attacks_;									//< 攻撃クラスのマップ
	std::unordered_map<BossAttackType, BossAnimType>				 attackAnimTable_;
};
