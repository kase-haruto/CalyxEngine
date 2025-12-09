#include "PlayerDamageHandler.h"

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>

// game
#include <Game/3dObject/Actor/Player/Player.h>
#include <Engine/Objects/Collider/Collider.h>

PlayerDamageHandler::PlayerDamageHandler() = default;
PlayerDamageHandler::~PlayerDamageHandler() = default;

/////////////////////////////////////////////////////////////////////////////////////////
// 初期化
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDamageHandler::Initialize(Player* owner) {
	owner_ = owner;
	invincibleTimer_ = 0.0f;
	invincibleBlinkAccum_ = 0.0f;
	invincibleBlinkState_ = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// 更新
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDamageHandler::Update(float dt) {
	UpdateInvincibility(dt);
}

/////////////////////////////////////////////////////////////////////////////////////////
// 被弾処理
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDamageHandler::OnHit(Collider* other) {
	if (!owner_) return;
	if (!other) return;

	// 無敵中なら無視
	if (IsInvincible()) return;

	// ===== ダメージ確定（Actor のライフを減らす）=====
	int32_t currentLife = owner_->GetLife();
	currentLife--;
	owner_->SetLife(currentLife);

	// ===== カメラシェイク =====
	if (auto* cam = CameraManager::GetMain3d()) {
		cam->StartShake(0.5f, 0.8f);
	}

	// ===== 無敵付与 =====
	SetInvincibleFor(kHitIFrameSec);
}

/////////////////////////////////////////////////////////////////////////////////////////
// 無敵付与
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDamageHandler::SetInvincibleFor(float seconds) {
	if (seconds <= 0.0f) return;

	const bool wasInvincible = IsInvincible();
	invincibleTimer_ = (std::max)(invincibleTimer_, seconds);

	if (!wasInvincible && owner_) {
		invincibleBlinkAccum_ = 0.0f;
		invincibleBlinkState_ = false;
		owner_->SetDrawEnable(false);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// 無敵中か
/////////////////////////////////////////////////////////////////////////////////////////
bool PlayerDamageHandler::IsInvincible() const {
	return invincibleTimer_ > 0.0f;
}

void PlayerDamageHandler::RequestInvincible(float seconds) {
	if(seconds <= 0.0f) return;

	const bool wasInvincible = IsInvincible();
	invincibleTimer_ = (std::max)(invincibleTimer_, seconds);

	if(!wasInvincible && owner_) {
		invincibleBlinkAccum_ = 0.0f;
		invincibleBlinkState_ = false;
		owner_->SetDrawEnable(false);
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// 無敵更新（点滅処理）
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDamageHandler::UpdateInvincibility(float dt) {
	if (invincibleTimer_ <= 0.0f) return;

	invincibleTimer_ -= dt;
	if (invincibleTimer_ <= 0.0f) {
		invincibleTimer_      = 0.0f;
		invincibleBlinkAccum_ = 0.0f;
		invincibleBlinkState_ = true;

		if (owner_) {
			owner_->SetDrawEnable(true);
		}
		return;
	}

	// 無敵中は一定間隔で描画トグル
	invincibleBlinkAccum_ += dt;
	while (invincibleBlinkAccum_ >= kBlinkInterval) {
		invincibleBlinkAccum_ -= kBlinkInterval;
		invincibleBlinkState_ = !invincibleBlinkState_;

		if (owner_) {
			owner_->SetDrawEnable(invincibleBlinkState_);
		}
	}
}