#pragma once
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Game/Battle/Movement/FollowSpline/SplineFollower.h>

class Enemy;
class WorldTransform;

class EnemyMovementController {
public:
	enum class Mode {
		Active,
		StayInView,
		ExitFromView
	};

public:
	EnemyMovementController();
	~EnemyMovementController() = default;

	void Initialize(Enemy* owner);
	void Update(float dt);

	// StayInView を開始
	void StartStay(float duration);

	// Active に戻す
	void StartActive();

	// Exit 処理を開始（内部からも呼ばれる）
	void BeginExit();

	// ルート（スプライン）セット
	void SetRoute(const SplineData& route, const WorldTransform* playerTf);
	void SetPlayerTransform(const WorldTransform* playerTf);
	Mode GetMode() const { return mode_; }

private:
	// 更新処理
	void UpdateStay(float dt);
	void UpdateExit(float dt);
	void UpdateActive(float dt);

	// ステイ中のドリフト
	void UpdateCameraDrift(float dt);

	// LookAt プレイヤー
	void LookAtPlayer();

	// 退場完了判定
	bool CheckExitFinished() const;

private:
	Enemy* owner_ = nullptr;

	Mode mode_ = Mode::Active;

	float stayTimer_   = 0.0f;
	float maxStayTime_ = 3.0f;

	// ドリフト
	float camPhaseX_ = 0, camPhaseY_ = 0, camPhaseZ_ = 0;
	float driftAmpX_ = 15.0f, driftAmpY_ = 15.5f, driftAmpZ_ = 15.0f;
	float driftFreqX_ = 0.7f, driftFreqY_ = 0.85f, driftFreqZ_ = 0.35f;
	float driftMargin_ = 0.4f;

	Vector3 camAnchor_ = {0, 0, 40};

	// Exit
	bool	exitPrepared_  = false;
	Vector3 exitDirLocal_  = {0, 0, 0};
	float	exitSpeed_	   = 35.0f;
	float	exitOvershoot_ = 1.05f;

	// Spline
	SplineFollower mover_;
	SplineData	   moveRoute_;
	bool		   hasRoute_ = false;

	const WorldTransform* playerTf_ = nullptr;
};
