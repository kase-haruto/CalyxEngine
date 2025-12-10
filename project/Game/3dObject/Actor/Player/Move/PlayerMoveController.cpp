#include "PlayerMoveController.h"

PlayerMoveController::PlayerMoveController() =default;
PlayerMoveController::~PlayerMoveController() =default;

//////////////////////////////////////////////////////////////////
//  移動量を追加する
//////////////////////////////////////////////////////////////////
void PlayerMoveController::AddMove(const Vector3& delta) {
	pendingMove_ += delta;
}

//////////////////////////////////////////////////////////////////
//  ワールド変換に適用する
//////////////////////////////////////////////////////////////////
void PlayerMoveController::Apply(WorldTransform& wt) {
	// NaN / Inf 保険（Release対策）
	if(!std::isfinite(pendingMove_.x) ||
	   !std::isfinite(pendingMove_.y) ||
	   !std::isfinite(pendingMove_.z)){
		pendingMove_ = Vector3::Zero();
	   }

	wt.translation += pendingMove_;
	pendingMove_ = Vector3::Zero();
}