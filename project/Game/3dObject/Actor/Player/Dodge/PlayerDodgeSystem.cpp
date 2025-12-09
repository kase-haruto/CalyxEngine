#include "PlayerDodgeSystem.h"

#include "Game/3dObject/Actor/Player/Player.h"


PlayerDodgeSystem::PlayerDodgeSystem()  = default;
PlayerDodgeSystem::~PlayerDodgeSystem() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//			初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDodgeSystem::Initialize(Player* owner,const PlayerDodgeConfig& cfg) {
	owner_ = owner;
	cfg_   = cfg;
	state_ = DodgeState::Idle;
	timer_ = 0.0f;
	cooldown_ = 0.0f;
	timeAccum_ = 0.0f;
	lastInputTime_ = -9999.0f;
	perfectHintActive_ = false;
}

/////////////////////////////////////////////////////////////////////////////////////////
//			更新
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDodgeSystem::Update(float dt){
	timeAccum_ += dt;
	if (cooldown_ > 0.0f) cooldown_ -= dt;

	switch (state_) {
	case DodgeState::Idle:
		break;

		// 回避開始
	case DodgeState::Startup:
		timer_ += dt;
		if (timer_ >= cfg_.startup) {
			ChangeState(DodgeState::IFrame);
		}
		break;

		// 無敵時間
	case DodgeState::IFrame:
		timer_ += dt;
		if (timer_ >= cfg_.duration) {
			ChangeState(DodgeState::Recovery);
		}
		break;

		// 回避終了
	case DodgeState::Recovery:
		timer_ += dt;
		if (timer_ >= cfg_.recovery) {
			ChangeState(DodgeState::Idle);
		}
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//			回避要求
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDodgeSystem::RequestDodge() {
	if(!owner_)return;
	if(state_ != DodgeState::Idle)return;
	if(cooldown_ > 0.0f)return;

	// dodge方向設定
	if (cfg_.useCameraForward){
		if (auto* cam = CameraManager::GetMain3d()){
			Vector3 fwd = Vector3::Forward();
			if (fwd.LengthSquared() > 1e-6f) fwd = fwd.Normalize();
			dodgeDir_ = fwd;
		} else {
			dodgeDir_ = {0,0,1};
		}
	} else {
		dodgeDir_ = {0,0,1};
	}

	lastInputTime_ = timeAccum_;
	cooldown_      = cfg_.cooldown;

	if (onDodgeStart_) onDodgeStart_();

	if (perfectHintActive_){
		if (onPerfectDodge_) onPerfectDodge_();
		if (owner_) owner_->SetInvincibleFor(cfg_.invuln + cfg_.perfectInvulnBonus);
		ChangeState(DodgeState::IFrame);
		return;
	}

	if (owner_) owner_->SetInvincibleFor(cfg_.invuln);
	ChangeState(DodgeState::Startup);
}

/////////////////////////////////////////////////////////////////////////////////////////
//			状態変更
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerDodgeSystem::ChangeState(DodgeState next) {
	state_ = next;
	timer_ = 0.0f;

	if (state_ == DodgeState::Idle){
		if (onDodgeEnd_) onDodgeEnd_();
	}
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
bool                     PlayerDodgeSystem::IsDodging() const { return state_ != DodgeState::Idle; }
bool                     PlayerDodgeSystem::IsInIFrame() const { return state_ == DodgeState::IFrame; }
DodgeState               PlayerDodgeSystem::GetState() const { return state_; }
float                    PlayerDodgeSystem::GetStateTime() const { return timer_; }
const Vector3&           PlayerDodgeSystem::GetDodgeDir() const { return dodgeDir_; }
const PlayerDodgeConfig& PlayerDodgeSystem::GetConfig() const { return cfg_; }
void                     PlayerDodgeSystem::SetPerfectHintActive(bool v) { perfectHintActive_ = v; }
bool                     PlayerDodgeSystem::WouldBePerfectIfDodgedNow() const { return perfectHintActive_; }