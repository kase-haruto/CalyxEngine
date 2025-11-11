#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineData.h>
#include <Engine/objects/Collider/SphereCollider.h>

// game
#include <Game/Battle/Movement/FollowSpline/SplineFollower.h>
#include <Game/Battle/Shooting/Pattern/ShootPatternDetails.h>
#include <Game/Battle/Shooting/ShootingController/BulletEmitter.h>
#include <Game/Battle/Shooting/ShootingController/EnemyShootingController.h>

/* ========================================================================
/* enemy
/* ===================================================================== */
class Enemy
	: public Actor {
public:
	enum class DeathState {
		Alive,
		Dying,
		Dead
	};

	enum class EnemyBehaviorState {
		StayingInView,
		ExitingView,
		Active /*…*/
	};

public:
	Enemy();
	Enemy(const std::string& modelName, const std::string objName);
	virtual ~Enemy() override;

	void Initialize() override;
	void Update(float dt) override;

	void EnsurePatternBound();

	// stay-in-camera 制御
	void StartStayInCamera(float duration = 2.0f);

	// collision ---------------------------------------------------------//
	void OnCollisionEnter(Collider* other) override;
	void OnCollisionStay([[maybe_unused]] Collider* other) override {}
	void OnCollisionExit([[maybe_unused]] Collider* other) override {}

	// accessor ---------------------------------------------------------//
	int16_t GetScore() const;
	void	SetPosition(const Vector3& position) { worldTransform_.translation = position; }
	void	SetShootingController(std::unique_ptr<EnemyShootingController>);
	void	SetPlayerTransform(const WorldTransform* position);
	void	SetRouteSpline(const SplineData& data);

	void BindPath(const SplineData* path, float startT = 0.0f, bool loop = true, int arcSamplesPerSeg = 48) { mover_.BindPath(path, startT, loop, arcSamplesPerSeg); }
	void SetSpawnerAnchor(const WorldTransform* spawnerTf) { mover_.SetAnchor(spawnerTf, /*inheritPos=*/true, /*inheritRot=*/true); }
	void SetPathWorldSpeed(float mps) { mover_.SetWorldSpeed(mps); }
	void SetPathLookMode(SplineFollower::LookMode m) { mover_.SetLookMode(m); }
	void SetPathYOffset(float y) { mover_.SetYOffset(y); }
	void SetPathTarget(const WorldTransform* t) { mover_.SetTargetTransform(t); }
	bool PathFinished() const { return mover_.IsFinished(); }

	DeathState		  GetDeathState() const { return deathState_; }
	const Vector3	  GetCenterPos() const override;
	void			  SetGameplayEngaged(bool v) { gameplayEngaged_ = v; }
	bool			  IsGameplayEngaged() const { return gameplayEngaged_; }
	void			  SetPatternKind(BulletPatternKind k) { patternKind_ = k; }
	BulletPatternKind GetPatternKind() const { return patternKind_; }

protected:
	void		 Move();
	void		 Shoot();
	virtual void Die();

private:
	void BuildEmitterIfReady();
	void UpdateCameraSpaceDrift(float dt);
	void StayInView(float dt); // stay-in-camera 用移動処理
	void BeginExitFromCamera();
	void UpdateExitFromCamera(float dt);

private:
	int16_t score_ = 125; //< 撃破スコア

	BulletPatternKind			   patternKind_		= BulletPatternKind::AimedNWay;
	BulletPatternKind			   lastPatternKind_ = BulletPatternKind::Spiral;
	std::unique_ptr<IShootPattern> pattern_;

	const WorldTransform* playerTransform_ = nullptr;
	DeathState			  deathState_	   = DeathState::Alive;
	EnemyBehaviorState	  behaviorState_   = EnemyBehaviorState::StayingInView;

	SplineFollower mover_;						 //< 移動
	SplineData	   moveRoute_{};				 //< 経路データ
	Vector3		   deathRotateAxis_ = {0, 0, 1}; //< 傾く軸
	Vector3		   basePosition_{};				 //< サイン波の基準位置

	bool  hasRoute_		   = false; //< 移動ルートを取得済みか
	bool  gameplayEngaged_ = false; //<
	float waveAmplitude_   = 1.0f;	//< 振れ幅
	float waveSpeed_	   = 2.0f;	//< サイン波の速さ
	float deathTimer_	   = 0.0f;	//< 死亡演出用
	float deathLength_	   = 1.5f;	//< 倒れ終わるまでの秒数

	float camDriftAmpX_	  = 15.0f; // 画面左右方向の最大振れ幅（ローカル単位）
	float camDriftAmpY_	  = 15.5f; // 画面上下方向の最大振れ幅
	float camDriftAmpZ_	  = 15.0f; // 手前奥の振れ幅
	float camDriftFreqX_  = 0.7f;  // Xの周波数
	float camDriftFreqY_  = 0.85f; // Yの周波数
	float camDriftFreqZ_  = 0.35f; // Zの周波数
	float camDriftMargin_ = 0.4f;  // 画面端からの余白率（0〜1）

	float	exitSpeedLocal_ = 35.0f; // 退場スピード（カメラローカル単位/秒）
	float	exitOvershoot_	= 1.05f; // 画面外判定のオーバー率(>1で確実に外)
	Vector3 exitDirLocal_	= {0, 0, 0};
	bool	exitPrepared_	= false;

	Vector3 camAnchor_ = {0, 0, 40};								 // 基準ローカル位置（Zで距離を調整）
	float	camPhaseX_ = 0.0f, camPhaseY_ = 1.7f, camPhaseZ_ = 3.1f; // 初期位相

	// stay-in-camera 用
	float stayInViewTime_ = 0.0f;
	float maxStayTime_	  = 3.0f;

	std::unique_ptr<BulletEmitter>			 emitter_;			  //< 一度だけ生成して保持
	std::unique_ptr<EnemyShootingController> shootingController_; //< 下流コントローラ
	std::shared_ptr<FxObject>				 hitFx_;			  //< ヒットエフェクト
};