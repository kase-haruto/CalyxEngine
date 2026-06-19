#include "PlayerInput.h"

using namespace CalyxFoundation;

PlayerInput::PlayerInput() {
	ResetBindings();
}

void PlayerInput::Update() {
	// フレームごとに入力状態を作り直す。
	// 古い押下情報が残ると、ジャンプなどの一回だけ処理したい入力が連続発火してしまうため。
	state_ = {};

	// キーボードとゲームパッド十字キーのデジタル入力を取得する。
	CalyxEngine::Vector2 digitalMove = BuildDigitalMove();

	// 左スティックのアナログ入力を取得する。
	// Input側ですでにデッドゾーン処理済みの値が返る前提で扱う。
	CalyxEngine::Vector2 analogMove = Input::GetLeftStick();

	// デジタル入力とアナログ入力を合成する。
	// 片方だけでも動けるようにし、同時入力時は後段で長さを1に制限する。
	state_.move = ClampMoveLength(digitalMove + analogMove);

	// ジャンプは押された瞬間だけを状態に残す。
	// CharacterMovementComponent側で接地中かどうかを判定するため、ここでは入力事実だけを扱う。
	state_.jumpPressed = IsTriggerAction(InputAction::Jump) || IsTriggerGamepadAction(InputAction::Jump);

	// 攻撃も押された瞬間だけを状態に残す。
	// 現時点ではDemoPlayer側で未使用だが、入力責務として状態を作っておく。
	state_.attackPressed = IsTriggerAction(InputAction::Attack) || IsTriggerGamepadAction(InputAction::Attack);

	// ダッシュは押し続け状態として扱う。
	// 移動速度変更などの継続効果で使えるように、トリガーではなくPushを参照する。
	state_.dashHeld = IsPushAction(InputAction::Dash) || IsPushGamepadAction(InputAction::Dash);
}

void PlayerInput::ShowGui() {
}

void PlayerInput::ResetBindings() {
	keyboardBindings_ = {
		{InputAction::MoveForward, DIK_W},
		{InputAction::MoveBackward, DIK_S},
		{InputAction::MoveLeft, DIK_A},
		{InputAction::MoveRight, DIK_D},
		{InputAction::Jump, DIK_SPACE},
		{InputAction::Attack, DIK_K},
		{InputAction::Dash, DIK_LSHIFT},
	};

	gamepadBindings_ = {
		{InputAction::MoveForward, PadButton::DPAD_UP},
		{InputAction::MoveBackward, PadButton::DPAD_DOWN},
		{InputAction::MoveLeft, PadButton::DPAD_LEFT},
		{InputAction::MoveRight, PadButton::DPAD_RIGHT},
		{InputAction::Jump, PadButton::A},
		{InputAction::Attack, PadButton::X},
		{InputAction::Dash, PadButton::LB},
	};
}

CalyxEngine::Vector2 PlayerInput::BuildDigitalMove() const {
	CalyxEngine::Vector2 move{0.0f, 0.0f};

	// 前後入力をY成分へ加算する。
	// 前方向を正、後方向を負として、移動変換側がそのままワールドZへ割り当てられるようにする。
	if (IsPushAction(InputAction::MoveForward) || IsPushGamepadAction(InputAction::MoveForward)) {
		move.y += 1.0f;
	}
	if (IsPushAction(InputAction::MoveBackward) || IsPushGamepadAction(InputAction::MoveBackward)) {
		move.y -= 1.0f;
	}

	// 左右入力をX成分へ加算する。
	// 右方向を正、左方向を負として、入力状態の意味を明確に保つ。
	if (IsPushAction(InputAction::MoveRight) || IsPushGamepadAction(InputAction::MoveRight)) {
		move.x += 1.0f;
	}
	if (IsPushAction(InputAction::MoveLeft) || IsPushGamepadAction(InputAction::MoveLeft)) {
		move.x -= 1.0f;
	}

	return ClampMoveLength(move);
}

CalyxEngine::Vector2 PlayerInput::ClampMoveLength(const CalyxEngine::Vector2& move) const {
	// 何も入力されていない場合は、そのままゼロを返す。
	if (move.LengthSquared() <= 0.0f) {
		return {0.0f, 0.0f};
	}

	// 長さが1以下ならアナログ入力の強弱を維持する。
	if (move.LengthSquared() <= 1.0f) {
		return move;
	}

	// 複数入力で長さが1を超えた場合だけ正規化する。
	// 斜め移動やキーボード+スティック同時入力で速度が増えないようにする。
	CalyxEngine::Vector2 clamped = move;
	clamped.Normalize();
	return clamped;
}
