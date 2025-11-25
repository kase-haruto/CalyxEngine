#pragma once

/**
 * \brief ボスの攻撃インターフェース
 */
class IBossAttack {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	IBossAttack();
	virtual ~IBossAttack();

	/**
	 * \brief クールダウンの時間を返す
	 * \return クールダウン時間
	 */
	float GetCooldown() const;
	/**
	 * \brief 攻撃の実行が可能かを返す
	 * \return 実行可能ならtrue、不可能ならfalse
	 */
	virtual bool Execute(class Boss& boss,class BossShootingController& shootingController) const;
	/**
	 * \brief GUI表示
	 */
	virtual void ShowGui() = 0;

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	float coolDown_ = 0.0f;
};