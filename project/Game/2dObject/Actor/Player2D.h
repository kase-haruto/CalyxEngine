#pragma once

#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Objects/2D/Actor/Character2D.h>
#include <Engine/Foundation/Utility/Animation/SimpleAnimator.h>

/*-----------------------------------------------------------------------------------------
 * 2dオブジェクトを使用したプレイヤークラス
 * - タイトルやゲームオーバー画面での操作キャラクターとして使用
 *---------------------------------------------------------------------------------------*/
class Player2D final
	: public Calyx2D::Character2D,
	  public CalyxEngine::SerializableObject {
private:
	enum Player2dState {
		MoveL,
		MoveR,
		Attack,
	};

public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	Player2D();
	~Player2D() override;

	/**
	 * \brief 初期化
	 */
	void Initialize() override;
	/**
	 * \brief 更新処理
	 * \param deltaTime デルタタイム
	 */
	void Update(float deltaTime) override;
	/**
	 * \brief デバッグUIを表示
	 */
	void ShowGui() override;

	CalyxEngine::ParamPath GetParamPath() const override {
		return { CalyxEngine::ParamDomain::Game, "Player2D" };
	}

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	void InitializeSpriteAnimation()const;
	/**
	 * \brief シリアライズ可能パラメータの初期化
	 */
	void InitializeSerializableParm();
	/**
	 * \brief アニメーション変更処理
	 */
	void ChangeAnimation(Player2dState state) const;
	/**
	 * \brief 移動処理
	 */
	void Move();
	/**
	 * \brief 状態更新処理
	 */
	void StateUpdate(float dt);

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	Player2dState currentState_{};	//< 現在の状態
	Player2dState prevState_{};		//< 前回の状態

	float amplitude_ = 30.0f;		//< 移動振幅
	float attackCoolTime_ = 0.0f;	//< 攻撃クールダウン
	float attackTimer_ = 0.0f;		//< 攻撃タイマー

	CalyxMath::Vector2 basePos_;
	CalyxUtil::SimpleAnimator animator_;  // アニメーター
};
