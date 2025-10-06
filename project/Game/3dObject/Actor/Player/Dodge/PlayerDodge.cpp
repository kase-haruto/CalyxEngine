#include "PlayerDodge.h"
#include <Game/3dObject/Actor/Player/Player.h>

PlayerDodge::PlayerDodge() = default;

PlayerDodge::~PlayerDodge() = default;

void PlayerDodge::Initialize(Player* owner, const PlayerDodgeConfig& cfg) {
	owner_ = owner;
	cfg_ = cfg;
	state_ = DodgeState::Idle;
	timer_ = 0.0f;
	cooldown_ = 0.0f;
	timeAccum_ = 0.0f;
	lastInputTime_ = -9999.0f;
}

void PlayerDodge::Update(float dt) {
	timeAccum_ += dt;

	// 入力（キーボード or Pad）
	if (Input::GetInstance()->TriggerKey(cfg_.dodgeKey) ||
		Input::GetInstance()->TriggerGamepadButton(PAD_BUTTON::A)) {
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
				// 等速ステップ移動（distance / duration）
				const float speed = (cfg_.duration > 0.0f) ? (cfg_.distance / cfg_.duration) : 0.0f;
				MoveOwnerBy(dodgeDir_ * speed);

				timer_ += dt;
				// I-Frame の可視的な区間は duration まで。無敵だけ invuln にしたいならここで調整可
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

void PlayerDodge::RequestDodge() {
	if (!owner_) return;
	if (state_ != DodgeState::Idle) return;
	if (cooldown_ > 0.0f) return;

	// 方向決定
	if (cfg_.useCameraForward) {
		if (auto* cam = CameraManager::GetMain3d()) {
			Vector3 fwd = /*cam->GetForward()*/Vector3::Forward();
			if (fwd.LengthSquared() > 1e-6f) fwd = fwd.Normalize();
			dodgeDir_ = fwd;
		} else {
			dodgeDir_ = Vector3(0, 0, 1);
		}
	} else {
		// とりあえず前無垢
		dodgeDir_ = Vector3(0, 0, 1);
	}

	// 入力確定時刻（ジャスト判定用）
	lastInputTime_ = timeAccum_;
	ChangeState(DodgeState::Startup);
	cooldown_ = cfg_.cooldown;
	if (onDodgeStart_) onDodgeStart_();
}

bool PlayerDodge::HandlesHitNow() {
	// すでに I-Frame なら常に無効化
	if (IsInIFrame()) return true;

	// 入力時刻との差分でジャスト判定
	const float dtFromInput = timeAccum_ - lastInputTime_;

	const bool inBefore = (dtFromInput >= -cfg_.perfectWindowBefore && dtFromInput < 0.0f);
	const bool inAfter = (dtFromInput >= 0.0f && dtFromInput <= cfg_.perfectWindowAfter);

	// 当たる直前で押す
	if (inBefore || inAfter) {
		if (onPerfectDodge_) onPerfectDodge_();

		// 被弾が来た瞬間に Startup をスキップして即無敵へ
		if (cfg_.fastForwardToIFrameOnPerfect && state_ != DodgeState::IFrame) {
			ChangeState(DodgeState::IFrame);
		}
		return true; // ダメージ無効
	}

	// プレイヤー有利にするためStartup 中の被弾は“押した直後”扱いでジャストにしても良い
	if (state_ == DodgeState::Startup) {
		if (onPerfectDodge_) onPerfectDodge_();
		if (cfg_.fastForwardToIFrameOnPerfect) {
			ChangeState(DodgeState::IFrame);
		}
		return true; // ダメージ無効
	}

	// ここまで来たら通常被弾を通す
	return false;
}

void PlayerDodge::ChangeState(DodgeState next) {
	state_ = next;
	timer_ = 0.0f;

	// Idle に戻った瞬間に終了演出フック
	if (state_ == DodgeState::Idle) {
		if (onDodgeEnd_) onDodgeEnd_();
	}
}

void PlayerDodge::MoveOwnerBy(const Vector3& velocity) {
	if (owner_) owner_->MoveBy(velocity);
}
