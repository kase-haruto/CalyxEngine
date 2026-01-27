#include "PlayerDodgeSystem.h"

#include "Game/3dObject/Actor/Player/Player.h"

PlayerDodgeSystem::PlayerDodgeSystem()	= default;
PlayerDodgeSystem::~PlayerDodgeSystem() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//			初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDodgeSystem::Initialize() {
	cfg_.LoadParams();

	state_			   = DodgeState::Idle;
	timer_			   = 0.0f;
	cooldown_		   = 0.0f;
	timeAccum_		   = 0.0f;
	lastInputTime_	   = -9999.0f;
	perfectHintActive_ = false;
}

/////////////////////////////////////////////////////////////////////////////////////////
//			更新
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDodgeSystem::Update(float dt) {
	timeAccum_ += dt;
	if(cooldown_ > 0.0f) cooldown_ -= dt;

	switch(state_) {
	case DodgeState::Idle:
		break;

		// 回避開始
	case DodgeState::Startup:
		timer_ += dt;
		if(timer_ >= cfg_.startup) {
			ChangeState(DodgeState::IFrame);
		}
		break;

		// 無敵時間
	case DodgeState::IFrame:
		timer_ += dt;
		if(timer_ >= cfg_.duration) {
			ChangeState(DodgeState::Recovery);
		}
		break;

		// 回避終了
	case DodgeState::Recovery:
		timer_ += dt;
		if(timer_ >= cfg_.recovery) {
			ChangeState(DodgeState::Idle);
		}
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//			回避要求
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDodgeSystem::RequestDodge(const CalyxMath::Vector3& dir) {
	if(state_ != DodgeState::Idle) return;
	if(cooldown_ > 0.0f) return;

	lastInputTime_ = timeAccum_;
	cooldown_	   = cfg_.cooldown;
	isJustDodge_   = false;

	if(dir.LengthSquared() > 0.001f) {
		dodgeDir_ = dir.Normalize();
	}

	// 回避開始イベント
	if(onDodgeStart_) onDodgeStart_();

	// ジャスト回避成功
	if(perfectHintActive_) {
		isJustDodge_ = true;
		if(onPerfectDodge_) onPerfectDodge_();

		// 無敵は「要求」だけ出す
		if(onRequestInvincible_) {
			onRequestInvincible_(cfg_.invuln + cfg_.perfectInvulnBonus);
		}

		ChangeState(DodgeState::IFrame);
		return;
	}

	// 通常回避の無敵要求
	if(onRequestInvincible_) {
		onRequestInvincible_(cfg_.invuln);
	}

	ChangeState(DodgeState::Startup);
}

CalyxMath::Vector3 PlayerDodgeSystem::GetDodgeVelocity() const {
	// StartupとIFrame中に移動する
	if(state_ == DodgeState::Startup || state_ == DodgeState::IFrame) {
		// ジャスト回避以外は移動しない
		if(!isJustDodge_) return {0.0f, 0.0f, 0.0f};

		const float totalTime = cfg_.startup + cfg_.duration;
		if(totalTime > 0.0001f) {
			const float speed = cfg_.distance / totalTime;
			return dodgeDir_ * speed;
		}
	}
	return {0.0f, 0.0f, 0.0f};
}

/////////////////////////////////////////////////////////////////////////////////////////
//			状態変更
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDodgeSystem::ChangeState(DodgeState next) {
	state_ = next;
	timer_ = 0.0f;

	if(state_ == DodgeState::Idle) {
		if(onDodgeEnd_) onDodgeEnd_();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//			コールバック関数を設定する
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDodgeSystem::SetOnRequestInvincible(std::function<void(float)> fn) {
	onRequestInvincible_ = std::move(fn);
}

void PlayerDodgeSystem::ShowGui() {
	cfg_.ShowGui();
}

void PlayerDodgeSystem::SaveConfig() {
	cfg_.SaveParams();
}
void PlayerDodgeSystem::LoadConfig() {
	cfg_.LoadParams();
}

/////////////////////////////////////////////////////////////////////////////////////////
//			コールバック
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDodgeSystem::SetOnDodgeStart(Callback cb) { onDodgeStart_ = std::move(cb); }
void PlayerDodgeSystem::SetOnDodgeEnd(Callback cb) { onDodgeEnd_ = std::move(cb); }
void PlayerDodgeSystem::SetOnPerfectDodge(Callback cb) { onPerfectDodge_ = std::move(cb); }

/////////////////////////////////////////////////////////////////////////////////////////
//			accessor
/////////////////////////////////////////////////////////////////////////////////////////
bool					  PlayerDodgeSystem::IsDodging() const { return state_ != DodgeState::Idle; }
bool					  PlayerDodgeSystem::IsInIFrame() const { return state_ == DodgeState::IFrame; }
DodgeState				  PlayerDodgeSystem::GetState() const { return state_; }
float					  PlayerDodgeSystem::GetStateTime() const { return timer_; }
const CalyxMath::Vector3& PlayerDodgeSystem::GetDodgeDir() const { return dodgeDir_; }
const PlayerDodgeConfig&  PlayerDodgeSystem::GetConfig() const { return cfg_; }
void					  PlayerDodgeSystem::SetPerfectHintActive(bool v) { perfectHintActive_ = v; }
bool					  PlayerDodgeSystem::WouldBePerfectIfDodgedNow() const { return perfectHintActive_; }