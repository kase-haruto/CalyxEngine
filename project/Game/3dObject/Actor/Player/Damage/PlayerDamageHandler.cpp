#include "PlayerDamageHandler.h"

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>

// game
#include <Engine/Objects/Collider/Collider.h>
#include <Game/3dObject/Actor/Player/Player.h>

PlayerDamageHandler::PlayerDamageHandler()  = default;
PlayerDamageHandler::~PlayerDamageHandler() = default;

/////////////////////////////////////////////////////////////////////////////////////////
// 初期化
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDamageHandler::Initialize(const PlayerStateContext& context) {
	ctx_                  = context;
	invincibleTimer_      = 0.0f;
	invincibleBlinkAccum_ = 0.0f;
	invincibleBlinkState_ = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// 更新
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDamageHandler::Update(float dt) { UpdateInvincibility(dt); }

/////////////////////////////////////////////////////////////////////////////////////////
// 被弾処理
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDamageHandler::OnHit(Collider* other) {
	if(!other) return;

	// 無敵中なら無視
	if(IsInvincible()) return;

	// ===== ダメージ確定（Actor のライフを減らす）=====
	int32_t currentLife = ctx_.getLife();
	currentLife--;
	ctx_.setLife(currentLife);

	// ===== カメラシェイク =====
	if(auto* cam = CameraManager::GetMain3d()) { cam->StartShake(0.5f,0.8f); }

	// ===== 無敵付与 =====
	SetInvincibleFor(config_.kHitIFrameSec);
}

/////////////////////////////////////////////////////////////////////////////////////////
// 無敵付与
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDamageHandler::SetInvincibleFor(float seconds) {
	if(seconds <= 0.0f) return;

	const bool wasInvincible = IsInvincible();
	invincibleTimer_         = (std::max)(invincibleTimer_,seconds);

	if(!wasInvincible) {
		invincibleBlinkAccum_ = 0.0f;
		invincibleBlinkState_ = false;
		ctx_.setVisible(false);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// 無敵中か
/////////////////////////////////////////////////////////////////////////////////////////
bool PlayerDamageHandler::IsInvincible() const { return invincibleTimer_ > 0.0f; }

void PlayerDamageHandler::RequestInvincible(float seconds) {
	if(seconds <= 0.0f) return;

	const bool wasInvincible = IsInvincible();
	invincibleTimer_         = (std::max)(invincibleTimer_,seconds);

	if(!wasInvincible) {
		invincibleBlinkAccum_ = 0.0f;
		invincibleBlinkState_ = false;
		ctx_.setVisible(false);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// GUI 表示
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDamageHandler::ShowGUi() {
	config_.ShowGui();
}

void PlayerDamageHandler::SaveParam() {
	config_.SaveParams();
}

void PlayerDamageHandler::LoadParam() {
	config_.LoadParams();
}

/////////////////////////////////////////////////////////////////////////////////////////
// 無敵更新（点滅処理）
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDamageHandler::UpdateInvincibility(float dt) {
	if(invincibleTimer_ <= 0.0f) return;

	invincibleTimer_ -= dt;
	if(invincibleTimer_ <= 0.0f) {
		invincibleTimer_      = 0.0f;
		invincibleBlinkAccum_ = 0.0f;
		invincibleBlinkState_ = true;

		ctx_.setVisible(true);
		return;
	}

	// 無敵中は一定間隔で描画トグル
	invincibleBlinkAccum_ += dt;
	while(invincibleBlinkAccum_ >= config_.kBlinkInterval) {
		invincibleBlinkAccum_ -= config_.kBlinkInterval;
		invincibleBlinkState_ = !invincibleBlinkState_;

		ctx_.setVisible(invincibleBlinkState_);
	}
}