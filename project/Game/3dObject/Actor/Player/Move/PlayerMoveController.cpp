#include "PlayerMoveController.h"

// engine
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>

// c++
#include <algorithm>
#include <cmath>

PlayerMoveController::PlayerMoveController()  = default;
PlayerMoveController::~PlayerMoveController() = default;

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
	   !std::isfinite(pendingMove_.z)) {
		pendingMove_ = CalyxMath::Vector3::Zero();
	}

	wt.translation += pendingMove_;
	pendingMove_ = CalyxMath::Vector3::Zero();

	// --- 画面外クランプ処理 (NDC方式) ---
	Camera3d* cam = CameraManager::GetMain3d();
	if(!cam) return;

	// 1. ワールド座標をクリップ空間に変換
	CalyxMath::Vector3			worldPos = wt.GetWorldPosition();
	const CalyxMath::Matrix4x4& viewProj = cam->GetViewProjectionMatrix();
	CalyxMath::Vector4			clipPos	 = CalyxMath::Vector4::Transform(CalyxMath::Vector4(worldPos, 1.0f), viewProj);

	if(std::abs(clipPos.w) < 1e-5f) return;

	// 2. NDC座標へ (-1 to 1)
	CalyxMath::Vector3 ndcPos = {
		clipPos.x / clipPos.w,
		clipPos.y / clipPos.w,
		clipPos.z / clipPos.w};

	// 3. NDC空間でクランプ (少しマージンを持たせる)
	float margin  = 0.9f;
	bool  clamped = false;
	if(ndcPos.x < -margin) {
		ndcPos.x = -margin;
		clamped	 = true;
	}
	if(ndcPos.x > margin) {
		ndcPos.x = margin;
		clamped	 = true;
	}
	if(ndcPos.y < -margin) {
		ndcPos.y = -margin;
		clamped	 = true;
	}
	if(ndcPos.y > margin) {
		ndcPos.y = margin;
		clamped	 = true;
	}

	if(clamped) {
		// 4. クランプされたNDC座標をワールド座標に戻す (逆ViewProjection)
		CalyxMath::Matrix4x4 invViewProj = CalyxMath::Matrix4x4::Inverse(viewProj);
		// W値(深度)を維持するために元のclipPos.wを掛ける
		CalyxMath::Vector4 clampedClipPos(ndcPos.x * clipPos.w, ndcPos.y * clipPos.w, ndcPos.z * clipPos.w, clipPos.w);
		CalyxMath::Vector4 clampedWorldH = CalyxMath::Vector4::Transform(clampedClipPos, invViewProj);

		CalyxMath::Vector3 clampedWorldPos(clampedWorldH.x / clampedWorldH.w, clampedWorldH.y / clampedWorldH.w, clampedWorldH.z / clampedWorldH.w);

		// 5. 親がいる場合は親空間に戻す
		if(wt.parent) {
			CalyxMath::Matrix4x4 invParentMat = CalyxMath::Matrix4x4::Inverse(wt.parent->matrix.world);
			wt.translation					  = CalyxMath::Vector3::Transform(clampedWorldPos, invParentMat);
		} else {
			wt.translation = clampedWorldPos;
		}
	}
}