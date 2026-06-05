#pragma once

#include "IInputAction.h"
#include <Demo/3D/Actor/DemoPlayer/Details/DemoPlayerActionConfig.h>

/**
 * \brief プレイヤー入力管理クラス
 * - キーボード、ゲームパッドの入力をInputActionに統合
 * - キーバインディングは動的に変更可能
 */
class PlayerInput :
public BaseInputAction<InputAction> {
public:
	PlayerInput();
	~PlayerInput() override = default;

	/**
	 * \brief ImGuiでキーバインディングを表示・編集
	 */
	void ShowGui();

	/**
	 * \brief キーバインディングを初期化
	 */
	void ResetBindings() override;
};


