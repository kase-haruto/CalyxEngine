#include "PlayerMoveController.h"

PlayerMoveController::PlayerMoveController() =default;
PlayerMoveController::~PlayerMoveController() =default;

//////////////////////////////////////////////////////////////////
//  移動量を追加する
//////////////////////////////////////////////////////////////////
void PlayerMoveController::AddMove(const CalyxMath::Vector3& delta) {
	pendingMove_ += delta;
}

//////////////////////////////////////////////////////////////////
//  ワールド変換に適用する
//////////////////////////////////////////////////////////////////
void PlayerMoveController::Apply(WorldTransform& wt) {
	if(!std::isfinite(pendingMove_.x) ||
	   !std::isfinite(pendingMove_.y) ||
	   !std::isfinite(pendingMove_.z)){
		pendingMove_ = CalyxMath::Vector3::Zero();
	   }

	wt.translation += pendingMove_;
	pendingMove_ = CalyxMath::Vector3::Zero();
}