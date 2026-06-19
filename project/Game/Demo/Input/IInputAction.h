#pragma once

#include <Engine/Foundation/Input/Input.h>
#include <unordered_map>
#include <type_traits>

/**
 * \brief 入力アクションの基底インターフェース
 */
class IInputAction {
public:
	virtual ~IInputAction() = default;

	virtual void Update() = 0;
	virtual void ResetBindings() = 0;
};

/**
 * \brief 型安全な入力アクションのテンプレート基底クラス
 * 派生クラスで特定のアクションEnumを指定して使用する
 */
template<typename EnumType>
class BaseInputAction : public IInputAction {
	static_assert(std::is_enum_v<EnumType>, "EnumType must be an enum");
public:
	using KeyboardBindingType = std::unordered_map<EnumType, uint32_t>;
	using GamepadBindingType = std::unordered_map<EnumType, CalyxFoundation::PadButton>;

	BaseInputAction() = default;
	virtual ~BaseInputAction() = default;

	void Update() override {
	}

	// キーボード入力チェック
	bool IsPushAction(EnumType action) const {
		auto it = keyboardBindings_.find(action);
		if (it != keyboardBindings_.end()) {
			return CalyxFoundation::Input::PushKey(it->second);
		}
		return false;
	}

	bool IsTriggerAction(EnumType action) const {
		auto it = keyboardBindings_.find(action);
		if (it != keyboardBindings_.end()) {
			return CalyxFoundation::Input::TriggerKey(it->second);
		}
		return false;
	}

	// ゲームパッド入力チェック
	bool IsPushGamepadAction(EnumType action) const {
		auto it = gamepadBindings_.find(action);
		if (it != gamepadBindings_.end()) {
			return CalyxFoundation::Input::PushGamepadButton(it->second);
		}
		return false;
	}

	bool IsTriggerGamepadAction(EnumType action) const {
		auto it = gamepadBindings_.find(action);
		if (it != gamepadBindings_.end()) {
			return CalyxFoundation::Input::TriggerGamepadButton(it->second);
		}
		return false;
	}

	// キーバインディング管理
	void SetKeyboardBinding(EnumType action, uint32_t key) {
		keyboardBindings_[action] = key;
	}

	void SetGamepadBinding(EnumType action, CalyxFoundation::PadButton button) {
		gamepadBindings_[action] = button;
	}

	uint32_t GetKeyboardBinding(EnumType action) const {
		auto it = keyboardBindings_.find(action);
		if (it != keyboardBindings_.end()) {
			return it->second;
		}
		return 0; // Invalid key
	}

	CalyxFoundation::PadButton GetGamepadBinding(EnumType action) const {
		auto it = gamepadBindings_.find(action);
		if (it != gamepadBindings_.end()) {
			return it->second;
		}
		return CalyxFoundation::PadButton::A; // Default button
	}

protected:
	KeyboardBindingType keyboardBindings_;
	GamepadBindingType gamepadBindings_;
};

