#include "ReticleMover.h"

// engine
#include <Engine/Foundation/Input/Input.h>

ReticleMover::ReticleMover() = default;
ReticleMover::~ReticleMover() = default;

//////////////////////////////////////////////////////////////////////////////
//	更新
//////////////////////////////////////////////////////////////////////////////
void ReticleMover::Update(float speed,float dt) {
	auto* input = CalyxFoundation::Input::GetInstance();

	// 入力（-1..1）
	CalyxMath::Vector2 rs = input->GetRightStick();
	float stickDeadZone_ = 0.15f;
	// 追加デッドゾーン（Input側のデッドゾーンを通っても、
	// パッド差や揺れで微小値が残る/または逆に倒しが浅いと0になりがちなので、
	// ここで「倒した量に応じてスムーズに0→1へ」変換する）
	const float mag = rs.Length();
	if (mag <= stickDeadZone_) {
		moveOffset_ = {0.0f, 0.0f};
		return;
	}

	// deadZone 以降を 0..1 に再マップ（スムーズに動かす）
	const float t = (mag - stickDeadZone_) / (1.0f - stickDeadZone_);
	const float scale = std::clamp(t, 0.0f, 1.0f) / mag; // 方向は維持して長さを再正規化

	CalyxMath::Vector2 dir = { rs.x * scale, rs.y * scale }; // 長さ 0..1

	// 速度と dt
	moveOffset_ = dir * speed * dt;
}

//////////////////////////////////////////////////////////////////////////////
//	レティクル移動オフセット取得
//////////////////////////////////////////////////////////////////////////////
const CalyxMath::Vector2& ReticleMover::MoveOffset() const {
	return moveOffset_;
}
