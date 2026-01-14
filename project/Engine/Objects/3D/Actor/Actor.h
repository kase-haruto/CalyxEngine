#pragma once

// engine
#include <Engine/Objects/3D/Actor/BaseGameObject.h>

// c++
#include <cstdint>
#include <string>

/*-----------------------------------------------------------------------------------------
 * Actor
 * - ゲーム上の行動するキャラクター基底クラス
 * - 位置、速度、生存フラグ、ライフなどの共通パラメータを保持
 *---------------------------------------------------------------------------------------*/
class Actor
	: public BaseGameObject {
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	/**
	 * \brief コンストラクタ
	 */
	Actor() = default;
	/**
	 * \brief コンストラクタ
	 * \param modelName モデル名
	 * \param objectName オブジェクト名
	 */
	Actor(const std::string&		 modelName,
		  std::optional<std::string> objectName);
	/**
	 * \brief デストラクタ
	 */
	virtual ~Actor() override = default;

	// getter
	/**
	 * \brief 衝突半径を取得
	 * \return 半径
	 */
	float		  GetCollisionRadius() const;
	/**
	 * \brief 速度を取得
	 * \return 速度
	 */
	const CalyxMath::Vector3 GetVelocity() const { return velocity_; }
	/**
	 * \brief 生存フラグを取得
	 * \return 生存しているか
	 */
	bool		  GetIsAlive() const { return isAlive_; }
	/**
	 * \brief 生存フラグを設定
	 * \param isAlive 生存しているか
	 */
	void		  SetIsAlive(bool isAlive) { isAlive_ = isAlive; }
	/**
	 * \brief ライフを取得
	 * \return ライフ
	 */
	int32_t		  GetLife() const { return life_; }
	// setter
	/**
	 * \brief 座標を設定
	 * \param position 座標
	 */
	void  SetPosition(const CalyxMath::Vector3& position) { worldTransform_.translation = position; };
	/**
	 * \brief 移動速度を設定
	 * \param moveSpeed 速度
	 */
	void  SetMoveSpeed(float moveSpeed) { moveSpeed_ = moveSpeed; }
	/**
	 * \brief 移動速度を取得
	 * \return 速度
	 */
	float GetMoveSpeed() const { return moveSpeed_; }
	/**
	 * \brief 速度を設定
	 * \param velocity 速度
	 */
	void  SetVelocity(const CalyxMath::Vector3& velocity) { velocity_ = velocity; }
	/**
	 * \brief ライフを設定
	 * \param life ライフ
	 */
	void SetLife(int32_t life) { life_ = life; }

protected:
	//===================================================================*/
	//                   protected members
	//===================================================================*/
	float	moveSpeed_;			  //< 移動速度
	CalyxMath::Vector3 velocity_	  = {};	  //< 移動ベクトル
	CalyxMath::Vector3 acceleration_ = {};	  //< 加速度
	int32_t life_		  = 1;	  //< 体力 (0で死亡)
	bool	isAlive_	  = true; //< 生存フラグ
};