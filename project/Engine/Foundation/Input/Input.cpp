#include "Input.h"

// engine
#include <Engine/Application/System/CalyxCore.h>
#include <Engine/Application/System/Environment.h>

// c++
#include <algorithm>
#include <cassert>

// externals
#include <externals/imgui/imgui.h>
namespace CalyxFoundation {
	std::unique_ptr<Input> Input::instance_ = nullptr;

	//-----------------------------------------------------------------------------

	Input::~Input() {
		// DirectInputで作成したキーボード・マウスのみ解放
		if(keyboard_) {
			keyboard_->Unacquire();
			keyboard_.Reset();
		}
		if(mouse_) {
			mouse_->Unacquire();
			mouse_.Reset();
		}
		if(directInput_) {
			directInput_.Reset();
		}
	}

	//-----------------------------------------------------------------------------

	Input* Input::GetInstance() {
		if(!instance_) {
			instance_ = std::unique_ptr<Input>(new Input());
		}
		return instance_.get();
	}

	//-----------------------------------------------------------------------------

	void Input::Initialize() {
		GetInstance();
		instance_->DirectInputInitialize();

		POINT pt;
		if(GetCursorPos(&pt)) {
			ScreenToClient(CalyxEngine::CalyxCore::GetHWND(), &pt);
			instance_->mousePos_ = {static_cast<float>(pt.x), static_cast<float>(pt.y)};
		}
	}

	//-----------------------------------------------------------------------------

	void Input::Update() {
		instance_->mouseStatePre_ = instance_->mouseState_;
		instance_->KeyboardUpdate();
		instance_->MouseUpdate();
		instance_->GamepadUpdate();
	}

	//-----------------------------------------------------------------------------

	void Input::Finalize() {
		instance_.reset();
	}

	//-----------------------------------------------------------------------------

	void Input::ShowImGui() {
		ImGui::Begin("Input Debug");
		ImGui::Text("Mouse Pos: %.1f, %.1f", instance_->mousePos_.x, instance_->mousePos_.y);
		ImGui::Text("Mouse Wheel: %.2f", instance_->mouseWheel_);
		ImGui::Text("Left Trigger: %.2f", instance_->leftTrigger_);
		ImGui::Text("Right Trigger: %.2f", instance_->rightTrigger_);
		ImGui::Text("Left Stick: (%.2f, %.2f)", instance_->leftThumbX_, instance_->leftThumbY_);
		ImGui::Text("Right Stick: (%.2f, %.2f)", instance_->rightThumbX_, instance_->rightThumbY_);
		ImGui::End();
	}

	//-----------------------------------------------------------------------------

	void Input::DirectInputInitialize() {
		HRESULT hr = DirectInput8Create(
			CalyxEngine::CalyxCore::GetHinstance(),
			DIRECTINPUT_VERSION,
			IID_IDirectInput8,
			reinterpret_cast<void**>(directInput_.GetAddressOf()),
			nullptr);
		assert(SUCCEEDED(hr));

		// キーボード
		hr = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
		assert(SUCCEEDED(hr));
		hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
		assert(SUCCEEDED(hr));
		hr = keyboard_->SetCooperativeLevel(CalyxEngine::CalyxCore::GetHWND(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
		assert(SUCCEEDED(hr));

		// マウス
		hr = directInput_->CreateDevice(GUID_SysMouse, &mouse_, NULL);
		assert(SUCCEEDED(hr));
		hr = mouse_->SetDataFormat(&c_dfDIMouse);
		assert(SUCCEEDED(hr));
		hr = mouse_->SetCooperativeLevel(CalyxEngine::CalyxCore::GetHWND(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
		assert(SUCCEEDED(hr));
	}

	//-----------------------------------------------------------------------------

	void Input::KeyboardUpdate() {
		keyPre_	   = key_;
		HRESULT hr = keyboard_->GetDeviceState(sizeof(key_), key_.data());

		if(FAILED(hr)) {
			// 何度かトライしてみる
			while(hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
				hr = keyboard_->Acquire();
				if(SUCCEEDED(hr)) {
					hr = keyboard_->GetDeviceState(sizeof(key_), key_.data());
				}
			}

			// それでも失敗したら全キーオフ扱い
			if(FAILED(hr)) {
				key_.fill(0);
			}
		}
	}

	//-----------------------------------------------------------------------------

	bool Input::PushKey(uint32_t keyNum) {
		assert(keyNum < 256);
		return instance_->key_[keyNum] & 0x80;
	}

	bool Input::TriggerKey(uint32_t keyNum) {
		assert(keyNum < 256);
		return (instance_->key_[keyNum] & 0x80) && !(instance_->keyPre_[keyNum] & 0x80);
	}

	//-----------------------------------------------------------------------------

	void Input::MouseUpdate() {
		HRESULT hr = mouse_->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState_);
		while(hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
			hr = mouse_->Acquire();
			if(SUCCEEDED(hr)) {
				hr = mouse_->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState_);
			}
		}

		if(FAILED(hr)) {
			mousePos_ = {0.0f, 0.0f};
		} else {
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(CalyxEngine::CalyxCore::GetHWND(), &pt);
			mousePos_	= {static_cast<float>(pt.x), static_cast<float>(pt.y)};
			mouseWheel_ = static_cast<float>(mouseState_.lZ) / WHEEL_DELTA;
		}
	}

	bool Input::PushMouseButton(MouseButton button) {
		return (instance_->mouseState_.rgbButtons[static_cast<int>(button)] & 0x80) != 0;
	}

	bool Input::TriggerMouseButton(MouseButton button) {
		return PushMouseButton(button) && !((instance_->mouseStatePre_.rgbButtons[static_cast<int>(button)] & 0x80) != 0);
	}

	bool Input::ReleaseMouseButton(MouseButton button) {
		return !PushMouseButton(button) && ((instance_->mouseStatePre_.rgbButtons[static_cast<int>(button)] & 0x80) != 0);
	}

	CalyxMath::Vector2 Input::GetMousePosition() {
		return instance_->mousePos_;
	}

	CalyxMath::Vector2 Input::GetMousePosInDebugWindow() {
		CalyxMath::Vector2 m_ImagePos  = CalyxMath::Vector2(0, 38);
		CalyxMath::Vector2 m_ImageSize = kExecuteWindowSize;
		CalyxMath::Vector2 m_GameSize  = kWindowSize;

		CalyxMath::Vector2 mousePos	 = GetMousePosition();
		float			   relativeX = mousePos.x - m_ImagePos.x;
		float			   relativeY = mousePos.y - m_ImagePos.y;

		float scaleX = m_GameSize.x / m_ImageSize.x;
		float scaleY = m_GameSize.y / m_ImageSize.y;

		return CalyxMath::Vector2(relativeX * scaleX, relativeY * scaleY);
	}

	float Input::GetMouseWheel() {
		return instance_->mouseWheel_;
	}

	CalyxMath::Vector2 Input::GetMouseDelta() {
		return CalyxMath::Vector2(
			static_cast<float>(instance_->mouseState_.lX),
			static_cast<float>(instance_->mouseState_.lY));
	}

	//-----------------------------------------------------------------------------

	void Input::GamepadUpdate() {
		gamepadStatePre_ = gamepadState_;

		XINPUT_STATE state;
		DWORD		 result = XInputGetState(0, &state);
		if(result == ERROR_SUCCESS) {
			gamepadState_ = state.Gamepad;

			leftThumbX_	 = NormalizeAxisInput(state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			leftThumbY_	 = NormalizeAxisInput(state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			rightThumbX_ = NormalizeAxisInput(state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			rightThumbY_ = NormalizeAxisInput(state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

			leftTrigger_  = state.Gamepad.bLeftTrigger / 255.0f;
			rightTrigger_ = state.Gamepad.bRightTrigger / 255.0f;
		} else {
			ZeroMemory(&gamepadState_, sizeof(XINPUT_GAMEPAD));
			leftThumbX_ = leftThumbY_ = rightThumbX_ = rightThumbY_ = leftTrigger_ = rightTrigger_ = 0.0f;
		}
	}

	float Input::NormalizeAxisInput(short value, short deadZone) {
		if(abs(value) < deadZone) return 0.0f;
		float norm = static_cast<float>(value) / 32767.0f;
		float dz   = static_cast<float>(deadZone) / 32767.0f;
		if(norm > 0)
			norm = (norm - dz) / (1.0f - dz);
		else
			norm = (norm + dz) / (1.0f - dz);
		return std::clamp(norm, -1.0f, 1.0f);
	}

	bool Input::PushGamepadButton(PadButton button) {
		return (instance_->gamepadState_.wButtons & static_cast<WORD>(button)) != 0;
	}

	bool Input::TriggerGamepadButton(PadButton button) {
		return PushGamepadButton(button) && !((instance_->gamepadStatePre_.wButtons & static_cast<WORD>(button)) != 0);
	}

	float Input::GetLeftTrigger() { return instance_->leftTrigger_; }
	float Input::GetRightTrigger() { return instance_->rightTrigger_; }

	CalyxMath::Vector2 Input::GetLeftStick() { return {instance_->leftThumbX_, instance_->leftThumbY_}; }
	CalyxMath::Vector2 Input::GetRightStick() { return {instance_->rightThumbX_, instance_->rightThumbY_}; }

	StickState Input::GetStickState() {
		return {GetLeftStick(), GetRightStick()};
	}

	bool Input::IsLeftStickMoved() {
		return std::sqrt(instance_->leftThumbX_ * instance_->leftThumbX_ + instance_->leftThumbY_ * instance_->leftThumbY_) > DEFAULT_DEAD_ZONE;
	}

} // namespace CalyxFoundation