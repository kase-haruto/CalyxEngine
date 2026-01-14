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

	/*-----------------------------------------------------------------------------------------
	 * Input
	 * - 入力管理クラス
	 * - キーボード、マウス、ゲームパッド（XInput）の入力を統合管理するシングルトン
	 *---------------------------------------------------------------------------------------*/
	class Input {
	public:
		/**
		 * \brief インスタンスを取得
		 * \return インスタンス
		 */
		static Input* GetInstance();

		// コピー禁止
		Input(const Input&)			   = delete;
		Input& operator=(const Input&) = delete;

		friend struct std::default_delete<Input>;

	public:
		/**
		 * \brief 初期化
		 */
		static void Initialize();
		/**
		 * \brief 更新
		 */
		static void Update();
		/**
		 * \brief 終了処理
		 */
		static void Finalize();
		/**
		 * \brief ImGui表示
		 */
		static void ShowImGui();

		// キーボード
		/**
		 * \brief キーが押されているか
		 * \param keyNum キー番号（DIK_XXX）
		 * \return 押されているか
		 */
		static bool PushKey(uint32_t keyNum);
		/**
		 * \brief キーが押された瞬間か
		 * \param keyNum キー番号（DIK_XXX）
		 * \return 押された瞬間か
		 */
		static bool TriggerKey(uint32_t keyNum);

		// マウス
		/**
		 * \brief マウスボタンが押されているか
		 * \param button ボタン
		 * \return 押されているか
		 */
		static bool				  PushMouseButton(MouseButton button);
		/**
		 * \brief マウスボタンが押された瞬間か
		 * \param button ボタン
		 * \return 押された瞬間か
		 */
		static bool				  TriggerMouseButton(MouseButton button);
		/**
		 * \brief マウスボタンが離された瞬間か
		 * \param button ボタン
		 * \return 離された瞬間か
		 */
		static bool				  ReleaseMouseButton(MouseButton button);
		/**
		 * \brief マウス座標を取得
		 * \return 座標
		 */
		static CalyxMath::Vector2 GetMousePosition();
		/**
		 * \brief デバッグウィンドウ内でのマウス座標を取得
		 * \return 座標
		 */
		static CalyxMath::Vector2 GetMousePosInDebugWindow();
		/**
		 * \brief マウスホイールの回転量を取得
		 * \return 回転量
		 */
		static float			  GetMouseWheel();
		/**
		 * \brief マウスの移動量を取得
		 * \return 移動量
		 */
		static CalyxMath::Vector2 GetMouseDelta();

		// ゲームパッド
		/**
		 * \brief ゲームパッドボタンが押されているか
		 * \param button ボタン
		 * \return 押されているか
		 */
		static bool				  PushGamepadButton(PadButton button);
		/**
		 * \brief ゲームパッドボタンが押された瞬間か
		 * \param button ボタン
		 * \return 押された瞬間か
		 */
		static bool				  TriggerGamepadButton(PadButton button);
		/**
		 * \brief 左トリガーの押し込み量を取得
		 * \return 押し込み量(0.0〜1.0)
		 */
		static float			  GetLeftTrigger();
		/**
		 * \brief 右トリガーの押し込み量を取得
		 * \return 押し込み量(0.0〜1.0)
		 */
		static float			  GetRightTrigger();
		/**
		 * \brief 左スティックの入力を取得
		 * \return 入力(X, Y)
		 */
		static CalyxMath::Vector2 GetLeftStick();
		/**
		 * \brief 右スティックの入力を取得
		 * \return 入力(X, Y)
		 */
		static CalyxMath::Vector2 GetRightStick();
		/**
		 * \brief 両スティックの状態を取得
		 * \return スティック状態
		 */
		static StickState		  GetStickState();
		/**
		 * \brief 左スティックが動かされているか
		 * \return 動かされているか
		 */
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
		//===================================================================*/
		//                    private member variables
		//===================================================================*/
		static std::unique_ptr<Input> instance_; //< インスタンス

		ComPtr<IDirectInput8> directInput_ = nullptr; //< DirectInputインスタンス
		ComPtr<IDirectInputDevice8> keyboard_ = nullptr; //< キーボードデバイス
		std::array<BYTE, 256> key_ = {}; //< 現在のキー状態
		std::array<BYTE, 256> keyPre_ = {}; //< 前回のキー状態

		ComPtr<IDirectInputDevice8> mouse_ = nullptr; //< マウスデバイス
		DIMOUSESTATE mouseState_ = {}; //< 現在のマウス状態
		DIMOUSESTATE mouseStatePre_ = {}; //< 前回のマウス状態
		CalyxMath::Vector2 mousePos_ = {}; //< マウス座標
		float mouseWheel_ = 0.0f; //< マウスホイール回転量

		XINPUT_GAMEPAD gamepadState_ = {}; //< 現在のゲームパッド状態
		XINPUT_GAMEPAD gamepadStatePre_ = {}; //< 前回のゲームパッド状態
		float leftThumbX_ = 0.0f; //< 左スティックX
		float leftThumbY_ = 0.0f; //< 左スティックY
		float rightThumbX_ = 0.0f; //< 右スティックX
		float rightThumbY_ = 0.0f; //< 右スティックY
		float leftTrigger_ = 0.0f; //< 左トリガー
		float rightTrigger_ = 0.0f; //< 右トリガー
	};
} // namespace CalyxFoundation