#include "PlayerDodge.h"
#include <Game/3dObject/Actor/Player/Player.h>
#include <Engine/Foundation/Clock/ClockManager.h>

void PlayerDodge::Initialize(Player* owner, const PlayerDodgeConfig& cfg) {
	owner_ = owner;
	cfg_ = cfg;
	state_ = DodgeState::Idle;
	timer_ = 0.0f;
	cooldown_ = 0.0f;
	timeAccum_ = 0.0f;
	lastInputTime_ = -9999.0f;
	perfectHintActive_ = false;
}

void PlayerDodge::Update(float dt) {
	timeAccum_ += dt;

	if (Input::GetInstance()->TriggerKey(cfg_.dodgeKey) ||
		Input::GetInstance()->TriggerGamepadButton(PadButton::X)) {
		RequestDodge();
	}

	if (cooldown_ > 0.0f) cooldown_ -= dt;

	switch (state_) {
		case DodgeState::Idle:
			break;

		case DodgeState::Startup:
			timer_ += dt;
			if (timer_ >= cfg_.startup) {
				ChangeState(DodgeState::IFrame);
			}
			break;

		case DodgeState::IFrame:
			{
				if (!cfg_.useCustomCurve) {
					const float speed = (cfg_.duration > 0.0f) ? (cfg_.distance / cfg_.duration) : 0.0f;
					MoveOwnerBy(dodgeDir_ * speed);
				}
				timer_ += dt;
				if (timer_ >= cfg_.duration) {
					ChangeState(DodgeState::Recovery);
				}
				break;
			}

		case DodgeState::Recovery:
			timer_ += dt;
			if (timer_ >= cfg_.recovery) {
				ChangeState(DodgeState::Idle);
			}
			break;
	}
}

void PlayerDodge::RequestDodge(){
	if (!owner_) return;
	if (state_ != DodgeState::Idle) return;
	if (cooldown_ > 0.0f) return;

	// forward の決定
	if (cfg_.useCameraForward){
		if (auto* cam = CameraManager::GetMain3d()){
			Vector3 fwd = Vector3::Forward();
			if (fwd.LengthSquared() > 1e-6f) fwd = fwd.Normalize();
			dodgeDir_ = fwd;
		} else{
			dodgeDir_ = Vector3(0, 0, 1);
		}
	} else{
		dodgeDir_ = Vector3(0, 0, 1);
	}

	lastInputTime_ = timeAccum_;
	cooldown_ = cfg_.cooldown;

	if (onDodgeStart_) onDodgeStart_();

	if (perfectHintActive_){
		// ジャスト成功：即IFrame + 追加無敵
		if (onPerfectDodge_) onPerfectDodge_();
		if (owner_) owner_->SetInvincibleFor(cfg_.invuln + cfg_.perfectInvulnBonus);
		ChangeState(DodgeState::IFrame);
		return;
	}

	//if (owner_) owner_->SetInvincibleFor(cfg_.invuln);
	//ChangeState(DodgeState::Startup);
}

bool PlayerDodge::HandlesHitNow() {
	return IsInIFrame();
}

void PlayerDodge::ChangeState(DodgeState next) {
	state_ = next;
	timer_ = 0.0f;
	if (state_ == DodgeState::Idle) {
		if (onDodgeEnd_) onDodgeEnd_();
	}
}

void PlayerDodge::MoveOwnerBy(const Vector3& ) {
	//if (owner_) owner_->MoveBy(velocity);
}