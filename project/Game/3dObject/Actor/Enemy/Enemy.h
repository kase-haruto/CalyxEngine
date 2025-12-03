#pragma once

// engine
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineData.h>
#include <Engine/objects/Collider/SphereCollider.h>

// game
#include <Game/Battle/Shooting/Pattern/ShootPatternDetails.h>
#include <Game/Battle/Shooting/ShootingController/BulletEmitter.h>
#include <Game/Battle/Shooting/ShootingController/EnemyShootingController.h>

#include "Movement/EnemyMovementController.h"

class Enemy : public Actor {
public:
	enum class DeathState {
		Alive,
		Dying,
		Dead
	};

public:
	Enemy();
	Enemy(const std::string& modelName, const std::string objName);
	virtual ~Enemy() override;

	void Initialize() override;
	void Update(float dt) override;

	void EnsurePatternBound();

	// stay-in-camera → movementController に委譲
	void StartStayInCamera(float duration = 2.0f);

	// collision
	void OnCollisionEnter(Collider* other) override;
	void OnCollisionStay(Collider*) override {}
	void OnCollisionExit(Collider*) override {}

	// accessor
	int16_t GetScore() const;
	void	SetPosition(const Vector3& pos) { worldTransform_.translation = pos; }

	void SetShootingController(std::unique_ptr<EnemyShootingController>);
	void SetPlayerTransform(const WorldTransform* pos);
	void SetRouteSpline(const SplineData& data);

	DeathState	  GetDeathState() const { return deathState_; }
	const Vector3 GetCenterPos() const override;

	void SetGameplayEngaged(bool v) { gameplayEngaged_ = v; }
	bool IsGameplayEngaged() const { return gameplayEngaged_; }

	void			  SetPatternKind(BulletPatternKind k) { patternKind_ = k; }
	BulletPatternKind GetPatternKind() const { return patternKind_; }

protected:
	void		 Shoot();
	virtual void Die();

private:
	void BuildEmitterIfReady();

private:
	int16_t score_ = 125;

	BulletPatternKind			   patternKind_		= BulletPatternKind::AimedNWay;
	BulletPatternKind			   lastPatternKind_ = BulletPatternKind::Spiral;
	std::unique_ptr<IShootPattern> pattern_;

	const WorldTransform* playerTransform_ = nullptr;

	DeathState deathState_ = DeathState::Alive;

	// death animation
	Vector3 deathRotateAxis_ = {0, 0, 1};
	float	deathTimer_		 = 0.0f;
	float	deathLength_	 = 1.5f;

	bool gameplayEngaged_ = false;

	EnemyMovementController movement_;

	// shooting
	std::unique_ptr<BulletEmitter>			 emitter_;
	std::unique_ptr<EnemyShootingController> shootingController_;
	std::shared_ptr<FxObject> hitFx_;
};
