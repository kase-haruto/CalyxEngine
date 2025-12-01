#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Game/3dObject/Actor/Bullet/PlayerBullet/HomingBullet.h>

/**
 * \brief ボスのホーミング弾
 */
class BossHomingBullet final
	: public HomingBullet {
public:
	//===================================================================*/
	//		public methods
	//===================================================================*/
	BossHomingBullet();
	BossHomingBullet(const std::string& modelName, const std::string& name);
	~BossHomingBullet() override;

	/**
	 * \brief 更新
	 * \param dt デルタタイム
	 */
	void Update(float dt) override;
	/**
	 * \brief デバッグ用gui
	 */
	void DerivativeGui() override;

	//--------- accessor ---------------------------------------------------
	float GetHomingDelay() const { return homingDelay_; }
	void  SetHomingDelay(float delay) { homingDelay_ = delay; }
	float GetHomingTimer() const { return homingTimer_; }
	void  SetHomingTimer(float time) { homingTimer_ = time; }

private:
	//===================================================================*/
	//		private methods
	//===================================================================*/
	float homingDelay_	   = 0.5f; //< ホーミング開始までの遅延時間
	float homingTimer_	   = 0.0f; //< ホーミング継続時間
	float homingLimitTime_ = 1.0f; //< ホーミング継続時間の上限
};
