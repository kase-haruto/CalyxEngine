#include "PlayerInputHandler.h"

#include <Game/3dObject/Actor/Player/Player.h>

// engine
#include <Engine/Application/Input/Input.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		playerのinput処理更新
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerInputHandler::Update(Player& player, float dt){
	HandleMove(player);
	HandleReticle(player, dt);
	HandleShoot(player);
	//HandleLockOn(player);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		移動処理
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerInputHandler::HandleMove(Player& player){
	Vector3 moveVector = {0.0f, 0.0f, 0.0f};

	// キーボード入力
	if (Input::GetInstance()->PushKey(DIK_A)) moveVector.x -= 1.0f;
	if (Input::GetInstance()->PushKey(DIK_D)) moveVector.x += 1.0f;
	if (Input::GetInstance()->PushKey(DIK_W)) moveVector.y += 1.0f;
	if (Input::GetInstance()->PushKey(DIK_S)) moveVector.y -= 1.0f;

	// ゲームパッド左スティック入力
	Vector2 leftStick = Input::GetInstance()->GetLeftStick();
	moveVector.x += leftStick.x;
	moveVector.y += leftStick.y;

	// 正規化
	if (moveVector.Length() > 0.0f)
		moveVector = moveVector.Normalize();

	// 移動速度をかける
	moveVector *= player.GetMoveSpeed();

	// 移動と傾き更新
	player.UpdateTilt(moveVector);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		レティクル更新
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerInputHandler::HandleReticle(Player& player, float dt){
	constexpr float moveSpeed = 6.0f;
	constexpr float stickSensitivity = 300.0f;

	Vector3 offset = Vector3::Zero();
	if (Input::GetInstance()->PushKey(DIK_UP)) offset.y += 3.0f;
	if (Input::GetInstance()->PushKey(DIK_DOWN)) offset.y -= 3.0f;
	if (Input::GetInstance()->PushKey(DIK_LEFT)) offset.x -= 3.0f;
	if (Input::GetInstance()->PushKey(DIK_RIGHT)) offset.x += 3.0f;

	// ゲームパッド右スティック
	Vector2 rightStick = Input::GetInstance()->GetRightStick();
	offset.x += rightStick.x * stickSensitivity * dt;
	offset.y += rightStick.y * stickSensitivity * dt;

	// キーボードだけ正規化
	Vector3 keyboardOffset = offset;
	keyboardOffset.x -= rightStick.x * stickSensitivity * dt;
	keyboardOffset.y -= rightStick.y * stickSensitivity * dt;

	// キーボード入力がある場合のみ正規化して速度を調整
	if (keyboardOffset.Length() > 0.0f){
		keyboardOffset.Normalize();
		keyboardOffset *= moveSpeed * dt;
		offset.x = keyboardOffset.x + rightStick.x * stickSensitivity * dt;
		offset.y = keyboardOffset.y + rightStick.y * stickSensitivity * dt;
	}

	// レティクル移動
	player.MoveReticle(offset);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		発射処理
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerInputHandler::HandleShoot(Player& player){
	if (!player.GetShootCooldown().has_value()
		|| !player.GetMaxShootInterval().has_value()){
		return;
	}

	// クールダウン取得
	float cooldown = player.GetShootCooldown().value();
	const float maxInterval = player.GetMaxShootInterval().value();

	// スペースキー or 右肩ボタンで発射
	if ((Input::GetInstance()->PushKey(DIK_SPACE)
		|| Input::GetInstance()->PushGamepadButton(PadButton::RB))
		&& cooldown <= 0.0f){
		player.RequestShoot();
		cooldown = maxInterval;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ロックオン処理
/////////////////////////////////////////////////////////////////////////////////////////
void PlayerInputHandler::HandleLockOn(Player& player){
	if (Input::GetInstance()->PushGamepadButton(PadButton::LB)){
		player.RequestLockOn();
	} else{
		player.RequestLockOnTargetClear();
	}
}