#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Objects/3D/Actor/BaseGameObject.h>

// c++
#include <cstdint>
#include <string>

class Character :
	public BaseGameObject{
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	Character() = default;
	Character(const std::string& modelName);
	virtual ~Character() override = default;

	virtual void Initialize() override;
	virtual void Update(float dt) override;

	void KnockBack(const Vector3& direction, float force, float duration = 0.5f); // durationを追加

	/* ui =========================================*/
	virtual void ShowGui() override;

protected:
	//===================================================================*/
	//                   private methods
	//===================================================================*/
	float moveSpeed_ = 0.0f;    //* 移動速度
	Vector3 velocity_ = {};     //* 移動ベクトル
	Vector3 acceleration_ = {}; //* 加速度
	int32_t life_ = 1;          //* 体力 (0で死亡)
	bool isAlive_ = true;       //* 生存フラグ
	bool onGround_ = false;     //* 地面に接地しているか

	// ノックバック関連
	bool isKnockedBack_ = false;          // ノックバック中かどうか
	float knockbackDuration_ = 0.0f;      // ノックバックの残り時間
	float knockbackForceX_ = 0.0f;        // ノックバックの水平方向の力
	float knockbackForceZ_ = 0.0f;        // ノックバックの水平方向の力
	float friction_ = 5.0f;               // 水平方向の摩擦係数（必要に応じて調整）

	//===================================================================*/
	//                   getter/setter
	//===================================================================*/
public:
	void velocity(const Vector3& velocity){ velocity_ = velocity; }

	bool GetIsAlive() const{ return isAlive_; }

	bool GetOnGround() const{ return onGround_; }

	void SetPosition(const Vector3& position){ worldTransform_.translation = position; };

	void SetVelocity(const Vector3& velocity){ velocity_ = velocity; }

	const Vector3 GetVelocity()const{ return velocity_; }
};
