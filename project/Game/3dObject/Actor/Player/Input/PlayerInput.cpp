#include "PlayerInput.h"

#include "Engine/Application/Input/Input.h"


std::vector<PlayerCommand> PlayerInput::CollectCommands(float dt) {
	std::vector<PlayerCommand> cmds;

	CxMath::Vector3 move{};

	if(Input::GetInstance()->PushKey(DIK_A)) move.x -= 1.0f;
	if(Input::GetInstance()->PushKey(DIK_D)) move.x += 1.0f;
	if(Input::GetInstance()->PushKey(DIK_W)) move.y+= 1.0f;
	if(Input::GetInstance()->PushKey(DIK_S)) move.y -= 1.0f;

	Vector2 stick = Input::GetInstance()->GetLeftStick();
	move.x += stick.x;
	move.y += stick.y;

	if(move.LengthSquared() > 0.0f) {
		cmds.push_back({
			PlayerCommandType::Move,
			CmdMove{ move.Normalize() }
		});
	}

	// レティクル
	CxMath::Vector3 ret{};
	Vector2 rs = Input::GetInstance()->GetRightStick();
	ret.x = rs.x * 300.0f * dt;
	ret.y = rs.y * 300.0f * dt;
	if(ret.LengthSquared() > 0.0f) {
		cmds.push_back({
			PlayerCommandType::MoveReticle,
			CmdMove{ ret }
		});
	}

	if(Input::GetInstance()->TriggerKey(DIK_LSHIFT) ||
	   Input::GetInstance()->TriggerGamepadButton(PadButton::X)) {
		cmds.push_back({ PlayerCommandType::Dodge, {} });

	   }

	if(Input::GetInstance()->PushKey(DIK_SPACE) ||
	   Input::GetInstance()->PushGamepadButton(PadButton::RB)) {
		cmds.push_back({ PlayerCommandType::Shoot, {} });
	   }

	return cmds;
}