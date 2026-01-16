#pragma once

#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Foundation/Utility/Animation/SimpleAnimator.h>
#include <Engine/Objects/2D/Actor/Character2D.h>

/*-----------------------------------------------------------------------------------------
 * Player2D
 * -2dオブジェクトを使用したプレイヤークラス
 * - タイトルやゲームオーバー画面での操作キャラクターとして使用
 *---------------------------------------------------------------------------------------*/
class Player2D final
	: public Calyx2D::Character2D,
	  public CalyxEngine::SerializableObject {
private:
	enum Player2dState {
		MoveL,  ///< 左移動状態
		MoveR,  ///< 右移動状態
		Attack, ///< 攻撃状態
	};

public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	/** \brief コンストラクタ*/
	Player2D();
	/** \brief デストラクタ*/
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

	/**
	 * \brief シリアライズの参照パスを取得
	 * \return パラメータパス
	 */
	CalyxEngine::ParamPath GetParamPath() const override {
		return {CalyxEngine::ParamDomain::Game, "Player2D"};
	}

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	/**
	 * \brief スプライトアニメーションの初期化
	 */
	void InitializeSpriteAnimation() const;
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
	 * \param dt デルタタイム
	 */
	void StateUpdate(float dt);

private:
	//===================================================================*/
	//			private members
	//===================================================================*/
	Player2dState currentState_{}; //< 現在の状態
	Player2dState prevState_{};	   //< 前回の状態

	float amplitude_	  = 30.0f; //< 移動振幅
	float attackCoolTime_ = 0.0f;  //< 攻撃クールダウン
	float attackTimer_	  = 0.0f;  //< 攻撃タイマー

	float moveSpeed_ = 80.0f;  //< 自動移動速度
	float moveRange_ = 128.0f; //< 移動可能範囲（左右）
	int	  moveDir_	 = 1;	   //< 1 = 右, -1 = 左

	CalyxMath::Vector2		  size_{64.0f, 64.0f}; //< サイズ
	CalyxMath::Vector2		  basePos_;			  //< 基準座標
	CalyxUtil::SimpleAnimator animator_;			  //< アニメーター
};
