#pragma once

#include "IInputAction.h"
#include <Demo/3D/Actor/DemoPlayer/Details/DemoPlayerActionConfig.h>
#include <Engine/Foundation/Math/Vector2.h>

/**
 * \brief プレイヤーの1フレーム分の入力状態
 * - 入力デバイスの種類を隠蔽し、ゲーム側はこの状態だけを参照する
 * - 移動はXが左右、Yが前後として扱う
 */
struct PlayerInputState {
	CalyxEngine::Vector2 move{0.0f, 0.0f}; //!< 移動入力（x=右、y=前）
	bool jumpPressed = false;              //!< ジャンプが押された瞬間か
	bool attackPressed = false;            //!< 攻撃が押された瞬間か
	bool dashHeld = false;                 //!< ダッシュ入力を押しているか
};

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
	 * \brief 現在フレームの入力状態を更新
	 */
	void Update() override;

	/**
	 * \brief 現在フレームの入力状態を取得
	 * \return 入力状態
	 */
	const PlayerInputState& GetState() const { return state_; }

	/**
	 * \brief ImGuiでキーバインディングを表示・編集
	 */
	void ShowGui();

	/**
	 * \brief キーバインディングを初期化
	 */
	void ResetBindings() override;

private:
	/**
	 * \brief キーボードと十字キーのデジタル移動入力を取得
	 * \return 正規化済みの移動入力
	 */
	CalyxEngine::Vector2 BuildDigitalMove() const;

	/**
	 * \brief 入力ベクトルの長さを最大1に制限
	 * \param move 制限対象の入力
	 * \return 長さを制限した入力
	 */
	CalyxEngine::Vector2 ClampMoveLength(const CalyxEngine::Vector2& move) const;

private:
	PlayerInputState state_{};
};


