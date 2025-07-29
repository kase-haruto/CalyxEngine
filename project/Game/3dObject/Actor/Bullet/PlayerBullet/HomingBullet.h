#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Game/3dObject/Actor/Bullet/BaseBullet.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>

class HomingBullet :
	public BaseBullet{
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	HomingBullet() = default;
	HomingBullet(const std::string& modelName, const std::string& name);
	~HomingBullet();
	void ShootInitialize(const Vector3& initPos, const Vector3& velocity)override;
	void Initialize()override;
	void SetTarget(const Actor* target);
	void Update(float dt) override;

	const Vector3 GetCenterPos() const override;

private:
	//===================================================================*/
	//						private methods
	//===================================================================*/

private:
	const Actor* target_ = nullptr;
	float homingSpeed_ = 2.0f;
	float rotateSpeed_ = 100.0f;

	std::shared_ptr<ParticleSystemObject> trailFx_;

};

