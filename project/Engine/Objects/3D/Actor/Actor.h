#pragma once

// engine
#include <Engine/Objects/3D/Actor/BaseGameObject.h>

// c++
#include <cstdint>
#include <string>

/**
 * ゲーム上の行動するキャラクター基底クラス
 */
class Actor
	: public BaseGameObject {
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	Actor() = default;
	Actor(const std::string&		 modelName,
		  std::optional<std::string> objectName);
	virtual ~Actor() override = default;

	// getter
	float		  GetCollisionRadius() const;
	const Vector3 GetVelocity() const { return velocity_; }
	bool		  GetIsAlive() const { return isAlive_; }
	int32_t		  GetLife() const { return life_; }
	// setter
	void SetPosition(const Vector3& position) { worldTransform_.translation = position; };
	void SetMoveSpeed(float moveSpeed) { moveSpeed_ = moveSpeed; }
	float GetMoveSpeed() const { return moveSpeed_; }
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }

protected:
	//===================================================================*/
	//                   private methods
	//===================================================================*/
	float	moveSpeed_;			  //< 移動速度
	Vector3 velocity_	  = {};	  //< 移動ベクトル
	Vector3 acceleration_ = {};	  //< 加速度
	int32_t life_		  = 1;	  //< 体力 (0で死亡)
	bool	isAlive_	  = true; //< 生存フラグ
};