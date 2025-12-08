#include "EnemyMovementController.h"
#include "../Enemy.h"
#include "Game/3dObject/Actor/Enemy/Enemy.h"
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Game/Battle/Movement/Formation/EnemyFormationController.h>

#include <algorithm>
#include <cmath>

namespace {
void ClampToFrustum(Vector3& p, float fovY, float aspect, float margin) {
	const float halfH = p.z * std::tan(fovY * 0.5f);
	const float halfW = halfH * aspect;
	p.x				  = std::clamp(p.x, -halfW * margin, halfW * margin);
	p.y				  = std::clamp(p.y, -halfH * margin, halfH * margin);
}

Vector3 ComputeDissolveDirection(int index, DissolvePattern pattern) {
	switch(pattern) {

	case DissolvePattern::AlternatingLeftRight:
		return Vector3(
				   (index % 2 == 0 ? -1.0f : +1.0f),
				   0.0f,
				   0.0f)
			.Normalize();

	case DissolvePattern::FourWay:
		switch(index % 4) {
		case 0:
			return Vector3(-1, +0.5f, 0.0f).Normalize();
		case 1:
			return Vector3(+1, +0.5f, 0.0f).Normalize();
		case 2:
			return Vector3(-1, -0.5f, 0.0f).Normalize();
		case 3:
			return Vector3(+1, -0.5f, 0.0f).Normalize();
		}
		return Vector3(0, 0, 1).Normalize();

	case DissolvePattern::VShape: {
		int pair = index / 2;
		int side = (index % 2 == 0) ? -1 : +1;
		return Vector3(
				   float(side) * (float(pair) + 1),
				   -(float(pair) + 1),
				   2.0f)
			.Normalize();
	}

	case DissolvePattern::Circle: {
		float angle = index * 0.8f;
		return Vector3(std::cos(angle), std::sin(angle), 0.0f).Normalize();
	}

	case DissolvePattern::StraightBack:
		return Vector3(0, 0, 1).Normalize();

	default:
		return Vector3(0, 0, 1).Normalize();
	}
}
} // namespace

EnemyMovementController::EnemyMovementController() {}

void EnemyMovementController::Initialize(Enemy* owner) {
	owner_ = owner;

	// ドリフト初期位相ランダム
	camPhaseX_ = Random::Generate<float>(0, 6.2831f);
	camPhaseY_ = Random::Generate<float>(0, 6.2831f);
	camPhaseZ_ = Random::Generate<float>(0, 6.2831f);
}

void EnemyMovementController::SetRoute(const SplineData& route, const WorldTransform* playerTf) {
	moveRoute_ = route;
	moveRoute_.BuildArcTable();

	mover_.BindPath(&moveRoute_);
	mover_.SetWorldSpeed(12.0f);
	mover_.SetLookMode(SplineFollower::LookMode::TowardsTarget);
	mover_.SetTargetTransform(playerTf);

	hasRoute_ = moveRoute_.SegmentCount() > 0;
	playerTf_ = playerTf;
}
void EnemyMovementController::SetPlayerTransform(const WorldTransform* playerTf) {
	playerTf_ = playerTf;
}

//==================================================================
//  Update
//==================================================================
void EnemyMovementController::Update(float dt) {
	switch(mode_) {
	case Mode::Entrance:
		UpdateEntrance(dt);
		break;

	case Mode::Dissolving:
		UpdateDissolve(dt);
		break;

	case Mode::Formation:
		UpdateFormation(dt);
		break;

	case Mode::StayInView:
		UpdateStay(dt);
		break;

	case Mode::ExitFromView:
		UpdateExit(dt);
		break;

	case Mode::Active:
	default:
		UpdateActive(dt);
		break;
	}
}

void EnemyMovementController::UpdateDissolve(float dt) {
	if(scatterVelocity_.LengthSquared() > 0.0001f) {
		owner_->GetWorldTransform().translation += scatterVelocity_ * dt;
		LookAtPlayer();
	}
}

//==================================================================
//  STAY
//==================================================================
void EnemyMovementController::StartStay(float duration) {
	mode_		 = Mode::StayInView;
	stayTimer_	 = 0.0f;
	maxStayTime_ = duration;

	if(auto cam = CameraManager::GetMain3dShared()) {
		owner_->GetWorldTransform().parent = &cam->GetWorldTransform();
	}

	camAnchor_ = Vector3(0, 0, 80);
}

void EnemyMovementController::UpdateStay(float dt) {
	stayTimer_ += dt;

	UpdateCameraDrift(dt);
	LookAtPlayer();

	if(stayTimer_ >= maxStayTime_) {
		BeginExit();
	}
}

void EnemyMovementController::UpdateCameraDrift([[maybe_unused]] float dt) {
	auto cam = CameraManager::GetMain3d();
	if(!cam || !owner_->GetWorldTransform().parent) return;

	float t = ClockManager::GetInstance()->GetTotalTime();

	float dx = std::sin((t + camPhaseX_) * driftFreqX_) * driftAmpX_;
	float dy = std::sin((t + camPhaseY_) * driftFreqY_) * driftAmpY_;
	float dz = std::sin((t + camPhaseZ_) * driftFreqZ_) * driftAmpZ_;

	Vector3 p = camAnchor_ + Vector3(dx, dy, dz);

	ClampToFrustum(p, cam->GetFovY(), cam->GetAspectRatio(), driftMargin_);
	owner_->SetTranslate(p);
}

//==================================================================
//  EXIT
//==================================================================
void EnemyMovementController::BeginExit() {
	mode_		  = Mode::ExitFromView;
	exitPrepared_ = false;
}

void EnemyMovementController::StartDissolve(int index, DissolvePattern pattern) {
	mode_ = Mode::Dissolving;

	float speed = owner_->GetMoveSpeed();

	Vector3 dir = ComputeDissolveDirection(index, pattern);

	scatterVelocity_ = dir.Normalize() * speed;

	formation_ = nullptr;
}

void EnemyMovementController::UpdateExit(float dt) {
	auto cam = CameraManager::GetMain3dShared();

	if(!cam) {
		owner_->SetIsAlive(false);
		return;
	}

	// 初回：外へ向かう方向を決める
	if(!exitPrepared_) {
		Vector3 p = owner_->GetWorldTransform().translation;

		float halfH = p.z * std::tan(cam->GetFovY() * 0.5f);
		float halfW = halfH * cam->GetAspectRatio();

		float side	= (Random::Generate<float>(0.f, 1.f) < 0.5f) ? -1.f : +1.f;
		float edgeX = side * halfW * 1.2f;
		float edgeY = Random::Generate<float>(-halfH * 0.8f, halfH * 0.8f);

		Vector3 target = {edgeX, edgeY, p.z};
		Vector3 dir	   = (target - p).Normalize();
		if(dir.LengthSquared() < 1e-8f) dir = {side, 0, 0};

		exitDirLocal_ = dir;
		exitPrepared_ = true;
	}

	// 移動
	Vector3 newPos = owner_->GetWorldTransform().translation + exitDirLocal_ * exitSpeed_ * dt;
	owner_->SetTranslate(newPos);
	LookAtPlayer();

	if(CheckExitFinished()) {
		owner_->SetIsAlive(false);
	}
}

bool EnemyMovementController::CheckExitFinished() const {
	auto		   cam = CameraManager::GetMain3dShared();
	const Vector3& p   = owner_->GetWorldTransform().translation;

	float halfH = p.z * std::tan(cam->GetFovY() * 0.5f);
	float halfW = halfH * cam->GetAspectRatio();

	bool outX = std::abs(p.x) > halfW * exitOvershoot_;
	bool outY = std::abs(p.y) > halfH * exitOvershoot_;

	return outX || outY;
}
void EnemyMovementController::StartEntranceToFormation(EnemyFormationController* formation, const Vector3& formationOffset, const Vector3& entranceStartWorld) {
	if(mode_ == Mode::Dissolving)
		return;
	formation_		 = formation;
	formationOffset_ = formationOffset;

	entranceStart_	= entranceStartWorld;
	entranceTarget_ = formation_->GetPosition() + formationOffset_;

	entranceTime_ = 0.0f;

	// 最初は画面外に配置
	owner_->SetTranslate(entranceStartWorld);

	// 親はカメラ
	if(auto cam = CameraManager::GetMain3dShared()) {
		owner_->GetWorldTransform().parent = &cam->GetWorldTransform();
	}

	mode_ = Mode::Entrance;
}

void EnemyMovementController::UpdateEntrance(float dt) {

	entranceTime_ += dt;
	float t = std::clamp(entranceTime_ / entranceLength_, 0.0f, 1.0f);

	// イージング（
	float ease = 1.0f - std::pow(1.0f - t, 3.0f); // EaseOutCubic

	Vector3 pos = Vector3::Lerp(entranceStart_, entranceTarget_, ease);
	owner_->SetTranslate(pos);
	LookAtPlayer();

	if(t >= 1.0f) {
		// 侵入完了 → Formation モードへ合流
		mode_ = Mode::Formation;
	}
}

//==================================================================
//  ACTIVE
//==================================================================
void EnemyMovementController::StartActive() {
	mode_ = Mode::Active;
}

void EnemyMovementController::UpdateActive(float dt) {
	// dissolve 時は scatterVelocity_ を使って移動する
	if(scatterVelocity_.LengthSquared() > 0.01f) {
		auto& wt = owner_->GetWorldTransform();
		wt.translation += scatterVelocity_ * dt;
		LookAtPlayer();
		return;
	}

	// dissolve でなければ通常の Active 動作
	if(hasRoute_) {
		mover_.Update(dt);
	}
	LookAtPlayer();
}

void EnemyMovementController::StartFormation(EnemyFormationController* formation, const Vector3& offset) {
	if(mode_ == Mode::Dissolving)
		return;

	formation_		 = formation;
	formationOffset_ = offset;
	formationPhase_	 = Random::Generate<float>(0.0f, 6.2831f);

	if(auto cam = CameraManager::GetMain3dShared()) {
		owner_->GetWorldTransform().parent = &cam->GetWorldTransform();
	}

	mode_ = Mode::Formation;
}

void EnemyMovementController::UpdateFormation(float /*dt*/) {
	if(!formation_ || !owner_) return;

	// FormationController の座標は「カメラローカル」として扱う
	Vector3 leaderPos = formation_->GetPosition();
	Vector3 off		  = formationOffset_;

	// 個体差の揺れを乗せてスターフォックス感
	float t = ClockManager::GetInstance()->GetTotalTime();
	off.x += std::sin(t * 2.1f + formationPhase_) * 1.5f;
	off.y += std::sin(t * 1.7f + formationPhase_) * 1.5f;

	Vector3 localPos = leaderPos + off;
	owner_->SetTranslate(localPos);

	// 見た目の向き：とりあえずプレイヤーを向かせる
	LookAtPlayer();
}

//==================================================================
//  LOOK AT
//==================================================================
void EnemyMovementController::LookAtPlayer() {
	if(!playerTf_) return;

	Vector3 target = playerTf_->GetWorldPosition();
	Vector3 myPos  = owner_->GetWorldPosition();
	Vector3 dir	   = (target - myPos).Normalize();
	if(dir.LengthSquared() < 1e-12f) return;

	float yaw	= std::atan2(dir.x, dir.z);
	float pitch = std::atan2(-dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z));

	owner_->SetRotate(Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch));
}
