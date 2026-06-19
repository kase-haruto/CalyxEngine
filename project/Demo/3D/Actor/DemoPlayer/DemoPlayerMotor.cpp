#include "DemoPlayerMotor.h"

#include "DemoPlayer.h"

#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Physics/Character/CharacterMovementComponent.h>

void DemoPlayerMotor::Update(DemoPlayer& player, const PlayerInputState& input, float /*dt*/) {
	// 入力状態はPlayerInputが作成済みなので、ここでは移動用の意味付けだけを行う。
	CalyxEngine::Vector3 worldDirection = BuildWorldMoveDirection(input.move);

	// 入力がないフレームでは移動命令を送らない。
	// CharacterMovementComponent側は前フレームの入力を保持しないため、停止状態になる。
	if (worldDirection.LengthSquared() > 0.0f) {
		// 移動方向をキャラクター移動コンポーネントへ渡す。
		// 実際の速度、床判定、重力、押し戻しはCharacterMovementComponentが担当する。
		player.GetCharacterMovement().AddMovementInput(worldDirection);

		// 見た目の向きだけを移動方向へ合わせる。
		// 物理判定の結果には影響させず、描画上の向きの責務として扱う。
		FaceMoveDirection(player, worldDirection);
	}

	// ジャンプ入力は「押された瞬間」だけを処理する。
	// 接地中かどうかの判定はCharacterMovementComponentに閉じ込める。
	if (input.jumpPressed) {
		player.GetCharacterMovement().Jump();
	}
}

CalyxEngine::Vector3 DemoPlayerMotor::BuildWorldMoveDirection(const CalyxEngine::Vector2& move) const {
	CalyxEngine::Vector3 right = CalyxEngine::Vector3::Right();
	CalyxEngine::Vector3 forward = CalyxEngine::Vector3::Forward();

	if(auto* camera = CameraManager::GetMain3d()) {
		const auto& cameraMatrix = camera->GetWorldTransform().matrix.world;

		right = {
			cameraMatrix.m[0][0],
			0.0f,
			cameraMatrix.m[0][2],
		};
		forward = {
			cameraMatrix.m[2][0],
			0.0f,
			cameraMatrix.m[2][2],
		};

		if(right.LengthSquared() > 0.0001f) {
			right = right.Normalize();
		} else {
			right = CalyxEngine::Vector3::Right();
		}

		if(forward.LengthSquared() > 0.0001f) {
			forward = forward.Normalize();
		} else {
			forward = CalyxEngine::Vector3::Forward();
		}
	}

	CalyxEngine::Vector3 direction = right * move.x + forward * move.y;

	// 斜め移動が速くならないように、長さが1を超える場合だけ正規化する。
	if (direction.LengthSquared() > 1.0f) {
		direction = direction.Normalize();
	}

	return direction;
}

void DemoPlayerMotor::FaceMoveDirection(DemoPlayer& player, const CalyxEngine::Vector3& worldDirection) const {
	// ゼロ方向では向きを決められないため、回転更新を行わない。
	if (worldDirection.LengthSquared() <= 0.0f) {
		return;
	}

	// モデルの基準前方をワールド前方として扱い、移動方向へ回転させる。
	CalyxEngine::Vector3 forward = CalyxEngine::Vector3::Forward();
	CalyxEngine::Vector3 to = worldDirection;
	to = to.Normalize();

	// Quaternion生成は既存のVector/Quaternionユーティリティへ委譲する。
	player.GetWorldTransform().rotation = CalyxEngine::Quaternion::FromToQuaternion(forward, to);
}
