#pragma once

#include <Demo/Input/PlayerInput.h>
#include <Engine/Foundation/Math/Vector3.h>

class DemoPlayer;

/**
 * \brief DemoPlayerの入力をキャラクター移動へ変換するクラス
 * - PlayerInputは入力状態の生成だけを担当する
 * - CharacterMovementComponentは物理移動と床判定だけを担当する
 * - このクラスは入力状態をワールド移動方向とジャンプ命令へ変換する責務を持つ
 */
class DemoPlayerMotor {
public:
	/**
	 * \brief 入力状態に基づいてプレイヤーへ移動命令を送る
	 * \param player 操作対象のプレイヤー
	 * \param input 現在フレームの入力状態
	 * \param dt 経過時間
	 */
	void Update(DemoPlayer& player, const PlayerInputState& input, float dt);

private:
	/**
	 * \brief 2D入力をカメラ基準のXZ平面移動方向へ変換
	 * \param move 入力方向（x=右、y=前）
	 * \return ワールド空間の移動方向
	 */
	CalyxEngine::Vector3 BuildWorldMoveDirection(const CalyxEngine::Vector2& move) const;

	/**
	 * \brief 移動方向へプレイヤーの向きを合わせる
	 * \param player 操作対象のプレイヤー
	 * \param worldDirection ワールド空間の移動方向
	 */
	void FaceMoveDirection(DemoPlayer& player, const CalyxEngine::Vector3& worldDirection) const;
};
