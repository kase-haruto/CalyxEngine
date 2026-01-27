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

void PlayerDangerSense::Initialize(const PlayerStateContext& ctx) {
	cfg_.LoadParams();
	ctx_ = ctx;

	cue_ = std::make_unique<Sprite>(cfg_.uiTex);
	cue_->Initialize(CalyxMath::Vector2{-1000.0f, -1000.0f}, cfg_.uiSize);
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

	CalyxMath::Vector3 playerPos = ctx_.getCenterPos();
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

void PlayerDangerSense::ShowGui() {
	cfg_.ShowGui();
}

void PlayerDangerSense::SaveParam() {
	cfg_.SaveParams();
}

void PlayerDangerSense::LoadParam() {
	cfg_.LoadParams();
}

// ============================================================
// 複数 BulletContainer を走査して危険判定
// ============================================================
bool PlayerDangerSense::ComputeDangerNearby(CalyxMath::Vector3& outPlayerPos) const {

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

			const CalyxMath::Vector3 bpos = b.GetWorldPosition();
			const CalyxMath::Vector3 bvel = b.GetVelocity() * b.GetMoveSpeed();
			const float              br   = b.GetCollisionRadius();
			const float              hitThreshold = playerR + br + cfg_.margin;

			// 弾が移動していない場合は無視
			float speedSq = bvel.LengthSquared();
			if(speedSq < 0.01f) return;

			// プレイヤー中心から見た弾の相対位置
			CalyxMath::Vector3 toBullet = bpos - outPlayerPos;

			// 最接近時間を求める
			// t = - (p . v) / |v|^2
			float t = -CalyxMath::Vector3::Dot(toBullet, bvel) / speedSq;

			// 現在から warnTime 秒後までの間に最接近するか判定
			if (t > 0.0f && t <= warnTime) {
				// 最接近時の距離（の二乗）を計算
				CalyxMath::Vector3 closestPos = bpos + bvel * t;
				float distSq = (closestPos - outPlayerPos).LengthSquared();

				if (distSq <= hitThreshold * hitThreshold) {
					found = true;
				}
			}
		});

		if(found) return true;
	}

	return false;
}

// ============================================================
// UI & Dodge flag 更新
// ============================================================
void PlayerDangerSense::ApplyDangerResult(bool danger, const CalyxMath::Vector3& playerPos) {

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
		CalyxMath::Vector2 screen = CalyxMath::WorldToScreen(playerPos);
		cue_->SetPosition(screen);
		cue_->SetIsVisible(true);
	} else {
		cue_->SetIsVisible(false);
	}
}