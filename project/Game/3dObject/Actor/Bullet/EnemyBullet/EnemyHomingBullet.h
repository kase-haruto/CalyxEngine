#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Game/3dObject/Actor/Bullet/BaseBullet.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>

class EnemyHomingBullet :
	public BaseBullet {
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	EnemyHomingBullet() = default;
	EnemyHomingBullet(const std::string& modelName, const std::string& name);
	~EnemyHomingBullet()override;
	void ShootInitialize(const Vector3& initPos, const Vector3& velocity)override;
	void Initialize()override;
	void OnShot();
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
	std::shared_ptr<ParticleSystemObject> shootFx_;
};

