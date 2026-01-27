#include "PlayerInput.h"

#include "Engine/Foundation/Input/Input.h"

using CalyxFoundation::Input;

std::vector<PlayerCommand> PlayerInput::CollectCommands(float /*dt*/) {
	std::vector<PlayerCommand> cmds;

	CalyxMath::Vector3 move{};

	if(Input::GetInstance()->PushKey(DIK_A)) move.x -= 1.0f;
	if(Input::GetInstance()->PushKey(DIK_D)) move.x += 1.0f;
	if(Input::GetInstance()->PushKey(DIK_W)) move.y += 1.0f;
	if(Input::GetInstance()->PushKey(DIK_S)) move.y -= 1.0f;

	CalyxMath::Vector2 stick = Input::GetInstance()->GetLeftStick();
	move.x += stick.x;
	move.y += stick.y;

	if(move.LengthSquared() > 0.0f) {
		cmds.push_back({PlayerCommandType::Move,
						CmdMove{move.Normalize()}});
	}

	if(Input::GetInstance()->TriggerKey(DIK_LSHIFT) ||
	   Input::GetInstance()->TriggerGamepadButton(CalyxFoundation::PadButton::X)) {

		CalyxMath::Vector3 dodgeDir = {0.0f, 0.0f, 0.0f};
		if(move.LengthSquared() > 0.001f) {
			dodgeDir = move.Normalize();
		}

		cmds.push_back({PlayerCommandType::Dodge, CmdDodge{dodgeDir}});
	}

	if(Input::GetInstance()->PushKey(DIK_SPACE) ||
	   Input::GetInstance()->PushGamepadButton(CalyxFoundation::PadButton::RB)) {
		cmds.push_back({PlayerCommandType::Shoot, {}});
	}

	return cmds;
}