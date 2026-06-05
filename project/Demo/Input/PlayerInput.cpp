#include "PlayerInput.h"

using namespace CalyxFoundation;

PlayerInput::PlayerInput() {
	ResetBindings();
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

