#pragma once

// engine
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineData.h>
#include <Engine/objects/Collider/SphereCollider.h>

// game
#include <Game/Battle/Shooting/Pattern/ShootPatternDetails.h>
#include "Movement/EnemyMovementController.h"
#include "Shoot/EnemyShootingAgent.h"

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

	// stay-in-camera → movementController に委譲
	void StartStayInCamera(float duration = 2.0f);
	EnemyMovementController* GetMovementController() { return &movement_; }

	void StartEntranceToFormation(
		EnemyFormationController* formation,
		const CalyxMath::Vector3&			  formationOffset,
		const CalyxMath::Vector3&			  entranceStartWorld);
	
	// collision
	void OnCollisionEnter(Collider* other) override;
	void OnCollisionStay(Collider*) override {}
	void OnCollisionExit(Collider*) override {}

	// accessor
	int16_t GetScore() const;
	void	SetPosition(const CalyxMath::Vector3& pos) { worldTransform_.translation = pos; }

	void SetShootingController(std::unique_ptr<EnemyShootingController>);
	void SetPlayerTransform(const WorldTransform* pos);
	void SetRouteSpline(const SplineData& data);

	DeathState	  GetDeathState() const { return deathState_; }
	const CalyxMath::Vector3 GetCenterPos() const override;

	void SetGameplayEngaged(bool v) { shooting_.SetGameplayEngaged(v); }
	bool IsGameplayEngaged() const { return shooting_.IsGameplayEngaged(); }

	void			  SetPatternKind(BulletPatternKind k) { shooting_.SetPatternKind(k); }
	BulletPatternKind GetPatternKind() const { return shooting_.GetPatternKind(); }

	/// 互換性用
	void EnsurePatternBound() { shooting_.EnsurePatternBound(); }

protected:
	virtual void Die();

private:
	int16_t score_ = 125;

	const WorldTransform* playerTransform_ = nullptr;

	BulletPatternKind			   patternKind_		= BulletPatternKind::SweepFan;
	BulletPatternKind			   lastPatternKind_ = BulletPatternKind::Spiral;
	std::unique_ptr<IShootPattern> pattern_;
	DeathState deathState_ = DeathState::Alive;

	// death animation
	CalyxMath::Vector3 deathRotateAxis_ = {0, 0, 1};
	float	deathTimer_		 = 0.0f;
	float	deathLength_	 = 1.5f;

	// movement
	EnemyMovementController movement_;
	// shooting
	EnemyShootingAgent shooting_;
	// effects
	std::shared_ptr<CalyxEffect::FxObject> hitFx_;
};
