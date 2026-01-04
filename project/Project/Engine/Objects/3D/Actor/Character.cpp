#include "Character.h"

#include <Engine/Application/System/System.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>

Character::Character(const std::string& modelName)
	: BaseGameObject(modelName){}

void Character::Initialize(){

}

void Character::Update(float deltaTime){
	const float gravity = -9.8f;             // 重力加速度
	const float terminalVelocity = -100.0f;   // 最大落下速度

	// 重力による加速度を計算
	acceleration_ = {0.0f, gravity, 0.0f};

	// 加速度を考慮して速度を更新
	velocity_ += acceleration_ * deltaTime;

	// 落下速度を最大落下速度に制限
	if (velocity_.y < terminalVelocity){
		velocity_.y = terminalVelocity;
	}

	// 地面に接触しているかどうかを判定
	if (worldTransform_.translation.y < 0.0f){
		onGround_ = true;
		worldTransform_.translation.y = 0.0f; // 地面上に位置を固定

	} else{
		onGround_ = false;
	}

	// ノックバック中の処理
	if (isKnockedBack_){
		knockbackDuration_ -= deltaTime;
		if (knockbackDuration_ <= 0.0f){
			isKnockedBack_ = false;
			knockbackDuration_ = 0.0f;

			// ノックバック終了時に水平方向の速度をリセット
			velocity_.x = 0.0f;
			velocity_.z = 0.0f;
		} else{
			// ノックバック中は水平方向の減衰を適用
			float deceleration = friction_ * deltaTime;

			// X方向の減衰
			if (knockbackForceX_ > 0.0f){
				knockbackForceX_ -= deceleration;
				if (knockbackForceX_ < 0.0f) knockbackForceX_ = 0.0f;
			} else if (knockbackForceX_ < 0.0f){
				knockbackForceX_ += deceleration;
				if (knockbackForceX_ > 0.0f) knockbackForceX_ = 0.0f;
			}

			// Z方向の減衰
			if (knockbackForceZ_ > 0.0f){
				knockbackForceZ_ -= deceleration;
				if (knockbackForceZ_ < 0.0f) knockbackForceZ_ = 0.0f;
			} else if (knockbackForceZ_ < 0.0f){
				knockbackForceZ_ += deceleration;
				if (knockbackForceZ_ > 0.0f) knockbackForceZ_ = 0.0f;
			}

			// 減衰後の速度を適用
			velocity_.x = knockbackForceX_;
			velocity_.z = knockbackForceZ_;
		}
	} else{
		// ノックバック中でない場合、通常の移動や摩擦を適用可能
		// 例えば、地面にいる場合に摩擦を適用して速度を減衰させる
		if (onGround_){
			float friction = 10.0f; // 地面摩擦係数（必要に応じて調整）
			float deceleration = friction * deltaTime;

			// X方向の減衰
			if (velocity_.x > 0.0f){
				velocity_.x -= deceleration;
				if (velocity_.x < 0.0f) velocity_.x = 0.0f;
			} else if (velocity_.x < 0.0f){
				velocity_.x += deceleration;
				if (velocity_.x > 0.0f) velocity_.x = 0.0f;
			}

			// Z方向の減衰
			if (velocity_.z > 0.0f){
				velocity_.z -= deceleration;
				if (velocity_.z < 0.0f) velocity_.z = 0.0f;
			} else if (velocity_.z < 0.0f){
				velocity_.z += deceleration;
				if (velocity_.z > 0.0f) velocity_.z = 0.0f;
			}
			// 接地している場合、Y方向の速度をリセット
			velocity_.y = 0.0f;
		}
	}

	// 最終的な位置更新
	worldTransform_.translation += velocity_ * deltaTime;
}

void Character::KnockBack(const Vector3& direction, float force, float duration){
	if (!isAlive_) return; // 生存していない場合は無視
	if (isKnockedBack_) return; // 既にノックバック中の場合は無視（必要に応じて再適用を許可）

	Vector3 normalizedDir = direction.Normalize();

	// 水平ノックバックの速度を設定（加算ではなく設定）
	knockbackForceX_ = normalizedDir.x * force;
	knockbackForceZ_ = normalizedDir.z * force;

	// 垂直方向（Y軸）の速度を設定して「ぴょん」と跳ねる
	float bounceForce = 2.0f; // 必要に応じて調整

	// ノックバックの速度を設定
	velocity_.x = knockbackForceX_;
	velocity_.z = knockbackForceZ_;
	velocity_.y = bounceForce;

	// ノックバック状態を有効化
	isKnockedBack_ = true;
	knockbackDuration_ = duration;
}


void Character::ShowGui(){


}
