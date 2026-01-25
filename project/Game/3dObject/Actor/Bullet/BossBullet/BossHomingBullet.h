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
	//			public methods
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
	float GetHomingDelay() const { return paramData_.homingDelay; }
	void  SetHomingDelay(float delay) { paramData_.homingDelay = delay; }
	float GetHomingTimer() const { return homingTimer_; }
	void  SetHomingTimer(float time) { homingTimer_ = time; }

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/

	struct BossHomingParam :
		public EnemyHomingBulletParam {
		BossHomingParam();
		CalyxEngine::ParamPath GetParamPath() const override;

		float homingDelay; //< ホーミング開始までの遅延時間
		float collisionRadius;
		float lifeTime = 20.0f;
	} paramData_;

	float homingTimer_ = 0.0f; //< ホーミング継続時間


};
