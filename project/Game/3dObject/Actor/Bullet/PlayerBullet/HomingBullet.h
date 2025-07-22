#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Game/3dObject/Actor/Bullet/BaseBullet.h>


class HomingBullet :
	public BaseBullet{
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	HomingBullet() = default;
	HomingBullet(const std::string& modelName, const std::string& name);
	void SetTarget(const WorldTransform* target);
	void Update() override;

private:
	//===================================================================*/
	//						private methods
	//===================================================================*/

private:
	const WorldTransform* target_ = nullptr;
	float homingSpeed_ = 20.0f;
	float rotateSpeed_ = 5.0f;

};

