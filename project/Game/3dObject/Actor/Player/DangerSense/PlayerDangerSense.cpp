#include "PlayerDangerSense.h"

// engine
#include <Engine/Foundation/Utility/Func/CxUtils.h>

// game
#include <Game/3dObject/Actor/Player/Player.h>
#include <Game/3dObject/Actor/Player/Dodge/PlayerDodge.h>
#include <Game/3dObject/Actor/Enemy/Directory/EnemyDirectory.h>
#include <Game/3dObject/Actor/Enemy/Enemy.h>
#include <Game/3dObject/Actor/Bullet/Container/EnemyBulletContainer.h>
#include <Game/3dObject/Actor/Bullet/BaseBullet.h>

PlayerDangerSense::PlayerDangerSense() = default;

PlayerDangerSense::~PlayerDangerSense() = default;

void PlayerDangerSense::Initialize(Player* owner,PlayerDodge* dodge,const DangerSenseConfig& cfg) {
	owner_ = owner;
	dodge_ = dodge;
	cfg_   = cfg;

	cue_ = std::make_unique<Sprite>(cfg_.uiTex);
	cue_->Initialize(Vector2{-1000.0f,-1000.0f},cfg_.uiSize); // 初期は画面外
	cue_->SetAnchorPoint(Vector2{0.5f,0.5f});
	cue_->SetIsVisible(false);

	frameCounter_ = 0;
	lastDanger_   = false;
}

void PlayerDangerSense::Update(float /*dt*/) {
	if(!owner_ || !dir_) return;

	// スキャン間引き
	bool computeNow = true;
	if(cfg_.throttleFrames > 1) {
		frameCounter_ = (frameCounter_ + 1) % cfg_.throttleFrames;
		computeNow    = (frameCounter_ == 0);
	}

	Vector3 playerPos = owner_->GetCenterPos();
	bool    dangerNow = lastDanger_;

	if(computeNow) {
		dangerNow   = ComputeDangerNearby(playerPos);
		lastDanger_ = dangerNow;
	}

	ApplyDangerResult(dangerNow,playerPos);

	if(cue_) cue_->Update();
}

void PlayerDangerSense::SetEnemyBulletContainer(EnemyBulletContainer* bulletContainer) { bulletContainer_ = bulletContainer; }

bool PlayerDangerSense::ComputeDangerNearby(Vector3& outPlayerPos) const {
	outPlayerPos = owner_->GetCenterPos();

	const float playerR = owner_->GetCollisionRadius() + cfg_.playerInflate;

	// 生存敵スナップショット
	auto enemies = dir_->SnapshotAlive();

	for(auto& e : enemies) {
		if(!e) continue;

		if(!bulletContainer_) continue;

		// 全弾走査
		bool found = false;
		bulletContainer_->ForEachBullet([&](BaseBullet& b) {
			const float br    = b.GetCollisionRadius();
			const float range = playerR + br + cfg_.margin;

			// 遠距離カット
			const Vector3 bpos = b.GetWorldPosition();
			if(cfg_.maxCheckDistance > 0.0f) {
				const float roughDist = (bpos - outPlayerPos).Length();
				if(roughDist > cfg_.maxCheckDistance) return;
			}

			const float dist = (bpos - outPlayerPos).Length();
			if(dist < range) {
				found = true; // 1発でも近ければ危険
			}
		});

		if(found) return true;
	}

	return false;
}

void PlayerDangerSense::ApplyDangerResult(bool danger,const Vector3& playerPos) {

	float dt = ClockManager::GetInstance()->GetDeltaTime();

	// danger が true なら猶予時間をリセット
	if (danger) {
		dangerHold_ = cfg_.graceTime;
	} else {
		// 減衰させる
		dangerHold_ = (std::max)(0.0f, dangerHold_ - dt);
	}

	bool hint = (dangerHold_ > 0.0f);

	// PlayerDodge に通知
	if(dodge_) {
		dodge_->SetPerfectHintActive(hint);
	}

	// UI
	if(!cue_) return;
	if(hint) {
		Vector2 screen = Cx::Math::WorldToScreen(playerPos);
		cue_->SetPosition(screen);
		cue_->SetIsVisible(true);
	} else {
		cue_->SetIsVisible(false);
	}
}