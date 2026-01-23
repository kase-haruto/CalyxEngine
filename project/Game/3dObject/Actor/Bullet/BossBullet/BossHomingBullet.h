#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Game/3dObject/Actor/Bullet/EnemyBullet/BaseEnemyHomingBullet.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

/**
 * \brief ボスのホーミング弾
 */
class BossHomingBullet final
	: public BaseEnemyHomingBullet {
public:
	//===================================================================*/
	//		public methods
	//===================================================================*/
	BossHomingBullet();
	BossHomingBullet(const std::string& modelName,const std::string& name);
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
	float GetHomingDelay() const { return param_.homingDelay; }
	void  SetHomingDelay(float delay) { param_.homingDelay = delay; }
	float GetHomingTimer() const { return homingTimer_; }
	void  SetHomingTimer(float time) { homingTimer_ = time; }
	void  SetHomingLimit(float time) { param_.homingLimitTime = time; }

private:
	//===================================================================*/
	//		private methods
	//===================================================================*/

	struct BossHomingParam :
		public CalyxEngine::SerializableObject {
		BossHomingParam();
		CalyxEngine::ParamPath GetParamPath() const override;

		float homingDelay     = 0.5f; //< ホーミング開始までの遅延時間
		float homingLimitTime = 1.0f; //< ホーミング継続時間の上限
		float collisionRadius = 2.5f;
	} param_;

	float homingTimer_ = 0.0f; //< ホーミング継続時間


};
