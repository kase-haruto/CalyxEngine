#include "PlayerDangerSense.h"

// engine
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Utility/Func/CxUtils.h>

// game
#include "Game/3dObject/Actor/Player/Dodge/PlayerDodgeSystem.h"
#include <Game/3dObject/Actor/Bullet/BaseBullet.h>
#include <Game/3dObject/Actor/Bullet/Container/BulletContainer.h>
#include <Game/3dObject/Actor/Enemy/Directory/EnemyDirectory.h>
#include <Game/3dObject/Actor/Player/Player.h>

PlayerDangerSense::PlayerDangerSense()	= default;
PlayerDangerSense::~PlayerDangerSense() = default;

void PlayerDangerSense::Initialize(const PlayerStateContext& ctx, const DangerSenseConfig& cfg) {
	ctx_ = ctx;
	cfg_ = cfg;

	cue_ = std::make_unique<Sprite>(cfg_.uiTex);
	cue_->Initialize(Vector2{-1000.0f, -1000.0f}, cfg_.uiSize);
	cue_->SetAnchorPoint({0.5f, 0.5f});
	cue_->SetIsVisible(false);

	frameCounter_ = 0;
	lastDanger_	  = false;
}

void PlayerDangerSense::Update(float /*dt*/) {
	if(!dir_) return;

	// 間引き
	bool computeNow = true;
	if(cfg_.throttleFrames > 1) {
		frameCounter_ = (frameCounter_ + 1) % cfg_.throttleFrames;
		computeNow	  = (frameCounter_ == 0);
	}

	Vector3 playerPos = ctx_.getCenterPos();
	bool	dangerNow = lastDanger_;

	if(computeNow) {
		dangerNow	= ComputeDangerNearby(playerPos);
		lastDanger_ = dangerNow;
	}

	ApplyDangerResult(dangerNow, playerPos);

	if(cue_) cue_->Update();
}

// ============================================================
// 複数コンテナ登録
// ============================================================
void PlayerDangerSense::AddBulletContainer(const BulletContainer* container) {
	if(container) {
		bulletContainers_.push_back(container);
	}
}

// ============================================================
// 複数 BulletContainer を走査して危険判定
// ============================================================
bool PlayerDangerSense::ComputeDangerNearby(Vector3& outPlayerPos) const {

	outPlayerPos = ctx_.getCenterPos();

	const float playerR = ctx_.getCollisionRadius() + cfg_.playerInflate;
	// const float maxDist = cfg_.maxCheckDistance;

	// --- 重要：今出したい猶予 ---
	const float warnTime = 0.5f;

	for(auto* container : bulletContainers_) {
		if(!container) continue;

		bool found = false;

		container->ForEachBullet([&](BaseBullet& b) {
			if(!b.GetIsAlive()) return;

			Vector3 bpos	  = b.GetWorldPosition();
			Vector3 bvel	  = b.GetVelocity();
			float	br		  = b.GetCollisionRadius();
			float	safeRange = playerR + br + cfg_.margin;

			// 静止弾は無視
			float speed = b.GetMoveSpeed();
			if(speed < 0.01f) return;

			Vector3 toPlayer = outPlayerPos - bpos;
			float	distance = toPlayer.Length();

			// 弾の進行方向とプレイヤー方向の内積（正面のみ判定）
			float dot = Vector3::Dot(toPlayer.Normalize(), bvel.Normalize());
			if(dot < 0.2f) return;

			// 衝突までの時間（Time-To-Impact）
			float timeToImpact = (distance - safeRange) / speed;

			// 判定
			if(timeToImpact <= warnTime) {
				found = true;
			}
		});

		if(found) return true;
	}

	return false;
}

// ============================================================
// UI & Dodge flag 更新
// ============================================================
void PlayerDangerSense::ApplyDangerResult(bool danger, const Vector3& playerPos) {

	float dt = ClockManager::GetInstance()->GetDeltaTime();

	if(danger) {
		dangerHold_ = cfg_.graceTime;
	} else {
		dangerHold_ = (std::max)(0.0f, dangerHold_ - dt);
	}

	bool hint = (dangerHold_ > 0.0f);

	ctx_.setPerfectHintActive(hint);

	// UI 更新
	if(!cue_) return;

	if(hint) {
		Vector2 screen = Cx::Math::WorldToScreen(playerPos);
		cue_->SetPosition(screen);
		cue_->SetIsVisible(true);
	} else {
		cue_->SetIsVisible(false);
	}
}