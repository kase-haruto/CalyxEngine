#include "BaseBossState.h"

#include "imgui/imgui.h"

#include <string>

BaseBossState::BaseBossState()	= default;
BaseBossState::~BaseBossState() = default;

namespace {
std::string TypeToString(BossStateType type) {
	switch(type) {
	case BossStateType::Idle:
		return "Idle";
	case BossStateType::Attack:
		return "Attack";
	case BossStateType::Defend:
		return "Defend";
	case BossStateType::Dead:
		return "Dead";
	default:
		return "None";
	}
}

} // namespace

void BaseBossState::ShowGui() {
	//現在のタイプを表示
	ImGui::SeparatorText(TypeToString(state_).c_str());
}
/////////////////////////////////////////////////////////////////////////////////////////
//		状態遷移リクエストの取得
/////////////////////////////////////////////////////////////////////////////////////////
const BaseBossState::TransitionRequest& BaseBossState::GetTransitionRequest() const {
	return request_;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ステートタイプの取得
/////////////////////////////////////////////////////////////////////////////////////////
BossStateType BaseBossState::GetStateType() const {
	return state_;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ステートタイプの設定
/////////////////////////////////////////////////////////////////////////////////////////
void BaseBossState::SetStatypeType(BossStateType type) {
	state_ = type;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		リクエストの送信
/////////////////////////////////////////////////////////////////////////////////////////
void BaseBossState::RequestChange(BossStateType next) {
	request_ = {TransitionRequest::Type::Change, next};
}
void BaseBossState::RequestPush(BossStateType next) {
	request_ = {TransitionRequest::Type::Push, next};
}
void BaseBossState::RequestPop() {
	request_ = {TransitionRequest::Type::Pop, BossStateType::None};
}