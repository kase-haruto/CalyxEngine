#pragma once

#define DIRECTINPUT_VERSION 0x0800

#include <Engine/Foundation/Math/Vector2.h>
#include <wrl.h>
#include <array>
#include <dinput.h>
#include <XInput.h>
#include <cmath>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "xinput.lib")

namespace CalyxFoundation {

	// ゲームパッドのデッドゾーンのデフォルト値
	constexpr float DEFAULT_DEAD_ZONE = 0.2f;

	// XInput準拠のゲームパッドボタン列挙（トリガーは別扱い）
	enum class PadButton : WORD {
		A		   = XINPUT_GAMEPAD_A,
		B		   = XINPUT_GAMEPAD_B,
		X		   = XINPUT_GAMEPAD_X,
		Y		   = XINPUT_GAMEPAD_Y,
		LB		   = XINPUT_GAMEPAD_LEFT_SHOULDER,
		RB		   = XINPUT_GAMEPAD_RIGHT_SHOULDER,
		BACK	   = XINPUT_GAMEPAD_BACK,
		START	   = XINPUT_GAMEPAD_START,
		L_STICK	   = XINPUT_GAMEPAD_LEFT_THUMB,
		R_STICK	   = XINPUT_GAMEPAD_RIGHT_THUMB,
		DPAD_UP	   = XINPUT_GAMEPAD_DPAD_UP,
		DPAD_DOWN  = XINPUT_GAMEPAD_DPAD_DOWN,
		DPAD_LEFT  = XINPUT_GAMEPAD_DPAD_LEFT,
		DPAD_RIGHT = XINPUT_GAMEPAD_DPAD_RIGHT,
		COUNT
	};

	enum class MouseButton {
		Left	 = 0,
		Right	 = 1,
		Middle	 = 2,
		XButton1 = 3,
		XButton2 = 4
	};

	using Microsoft::WRL::ComPtr;

	// スティック状態構造体
	struct StickState {
		CalyxMath::Vector2 leftStick;
		CalyxMath::Vector2 rightStick;
	};

	class Input {
	public:
		static Input* GetInstance();

		// コピー禁止
		Input(const Input&)			   = delete;
		Input& operator=(const Input&) = delete;

	public:
		static void Initialize();
		static void Update();
		static void Finalize();
		static void ShowImGui();

		// キーボード
		static bool PushKey(uint32_t keyNum);
		static bool TriggerKey(uint32_t keyNum);

		// マウス
		static bool				  PushMouseButton(MouseButton button);
		static bool				  TriggerMouseButton(MouseButton button);
		static bool				  ReleaseMouseButton(MouseButton button);
		static CalyxMath::Vector2 GetMousePosition();
		static CalyxMath::Vector2 GetMousePosInDebugWindow();
		static float			  GetMouseWheel();
		static CalyxMath::Vector2 GetMouseDelta();

		// ゲームパッド
		static bool				  PushGamepadButton(PadButton button);
		static bool				  TriggerGamepadButton(PadButton button);
		static float			  GetLeftTrigger();
		static float			  GetRightTrigger();
		static CalyxMath::Vector2 GetLeftStick();
		static CalyxMath::Vector2 GetRightStick();
		static StickState		  GetStickState();
		static bool				  IsLeftStickMoved();

	private:
		Input() = default;
		~Input();

		void  DirectInputInitialize();
		void  KeyboardUpdate();
		void  MouseUpdate();
		void  GamepadUpdate();
		float NormalizeAxisInput(short value, short deadZone);

	private:
		static Input* instance_;

		// DirectInputオブジェクト（キーボード・マウス用）
		ComPtr<IDirectInput8> directInput_ = nullptr;

		// キーボード
		ComPtr<IDirectInputDevice8> keyboard_ = nullptr;
		std::array<BYTE, 256>		key_{};
		std::array<BYTE, 256>		keyPre_{};

		// マウス
		ComPtr<IDirectInputDevice8> mouse_ = nullptr;
		DIMOUSESTATE				mouseState_{};
		DIMOUSESTATE				mouseStatePre_{};
		CalyxMath::Vector2			mousePos_{};
		float						mouseWheel_ = 0.0f;

		// ゲームパッド（XInput）
		XINPUT_GAMEPAD gamepadState_{};
		XINPUT_GAMEPAD gamepadStatePre_{};
		float		   leftThumbX_	 = 0.0f;
		float		   leftThumbY_	 = 0.0f;
		float		   rightThumbX_	 = 0.0f;
		float		   rightThumbY_	 = 0.0f;
		float		   leftTrigger_	 = 0.0f;
		float		   rightTrigger_ = 0.0f;
	};
} // namespace CalyxFoundation