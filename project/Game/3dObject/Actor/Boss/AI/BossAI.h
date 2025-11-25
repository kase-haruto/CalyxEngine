#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */

// c++
#include <memory>
#include <vector>

// fwd
class Boss;
class BossShootingController;
class IBossAttack;

/**
 * \brief ボスの行動を制御するAIクラス
 */
class BossAI {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	BossAI(Boss* owner,BossShootingController* shooter);
	~BossAI();

	void Update(float dt);
	void AddAttack(std::unique_ptr<IBossAttack> attack);
private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	Boss* owner_;
	BossShootingController* shooter_;
	std::vector<std::unique_ptr<IBossAttack>> attacks_;
	float cooldownTimer_ = 0.0f;
};