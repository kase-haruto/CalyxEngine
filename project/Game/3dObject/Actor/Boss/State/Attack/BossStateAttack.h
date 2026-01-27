#pragma once
#include "../Base/BaseBossState.h"
#include "Game/3dObject/Actor/Boss/Attack/IBossAttack.h"
#include "Game/3dObject/Actor/Boss/Details/BossAnimType.h"
#include "Game/3dObject/Actor/Boss/Details/BossAttackType.h"

#include <Engine/Foundation/Serialization/SerializableObject.h>

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
	struct BossAttackParam :
		public CalyxEngine::SerializableObject {

		BossAttackParam();
		CalyxEngine::ParamPath GetParamPath() const override;

		float maxAttackTime       = 3.0f; // 最大3秒で強制終了（必要に応じて変更）
		float repeatColldownLimit = 1.0f; // リピート時のクールダウン上限

	}param_;

	float timer_               = 0.0f;
	float repeatCooldownTimer_ = 0.0f; // リピート時のクールダウン

	BossAttackType                                                  attackType_ = BossAttackType::Punch; //< 攻撃タイプ
	std::unordered_map<BossAttackType,std::unique_ptr<IBossAttack>> attacks_;                            //< 攻撃クラスのマップ
	std::unordered_map<BossAttackType,BossAnimType>                 attackAnimTable_;                    //< 攻撃アニメーションテーブル
};