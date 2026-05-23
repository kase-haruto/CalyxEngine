#include "DiceProjectileObject.h"

#include <Game\StageGimmick\Gimmicks\DiceProjectile\DiceSocketObject.h>

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine\Foundation\Utility\Ease\CxEase.h>

REGISTER_SCENE_OBJECT(DiceProjectileObject)

DiceProjectileObject::DiceProjectileObject(
	const std::string&		   modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {

	worldTransform_.translation.y += 2.5f;
}

void DiceProjectileObject::OnCollisionEnter(Collider* other) {

	// ハンマー判定かどうか確認する
	// ギミックなどへの干渉 or 相手側に追加する
	BaseGameObject* otherObj = other->GetOwner();
	if(otherObj && other->GetType() != ColliderType::Type_PlayerAttack) {
		return;
	}
	if(isParry_) {
		return;
	}
	// 収納箱の座標を取得する
	targetPos_ = socket_->GetSocketPos();
	targetPos_.z -= socket_->GetWorldTransform().scale.z / 2.0f;  
	// 収納箱の数をプラスする
	socket_->AddDiceSocketCount();
	worldTransform_.scale = param_.hitScale;
	isParry_ = true;
}

void DiceProjectileObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}

	worldTransform_.scale = CalyxEngine::Vector3::One() * param_.scale;
	worldTransform_.inheritScale = false;
	isParry_ = false;
}

void DiceProjectileObject::SameNumberRotation() {
	auto rotation = socket_->GetSameNumberRotation();
	worldTransform_.rotation = CalyxEngine::Quaternion::Slerp(
		worldTransform_.rotation, rotation, 0.1f);
}

void DiceProjectileObject::ObjectUpdate(float dt) {

	// スケールを戻す処理
	ChangeScale();
	// 収納したら回転を戻す
	if(isSocket_) {
		// 回転を元に戻す
		SameNumberRotation();
		return;
	}

	// 回転の処理を加える
	if(!isParry_) {
		time_ += dt;
		if(time_ >= (std::numbers::pi_v<float> * 2.0f)) {
			time_ = 0.0f;
		}
		float angle				 = time_ * param_.rotateSpeed;
		auto  rotation			 = CalyxEngine::Quaternion::MakeRotateAxisQuaternion(CalyxEngine::Vector3::One(), angle);
		worldTransform_.rotation = rotation;
	}

	// 飛んでいなければ更新を飛ばす
	if(!isFlying_) {
		worldTransform_.scale = param_.scale;
		return;
	}

	// パラメータ方向に飛ばす
	if(isParry_) {

		// 初期化（isParry_に入った瞬間に1回だけ）
		if(!parryCurveInit_) {
			parryCurveInit_ = true;
			parryT_			= 0.0f;
			parryP0_		= worldTransform_.translation;
			parryP2_		= targetPos_;
			// 中点上方向オフセットで山なりにする
			CalyxEngine::Vector3 mid = (parryP0_ + parryP2_) * 0.5f;
			mid.y += 5.0f;
			if(parryP2_.x > parryP0_.x) {
				mid.x -= 5.0f;
			} else {
				mid.x += 5.0f;
			}
			// 距離に応じて高さを自動調整（好みで係数調整）
			float dist		= (parryP2_ - parryP0_).Length();
			float arcHeight = std::max(0.5f, dist * 0.25f);

			parryP1_ = mid + CalyxEngine::Vector3(0.0f, arcHeight, 0.0f);
		}

		parryT_ += (dt / std::max(0.0001f, param_.parryDuration));
		float t = std::clamp(parryT_, 0.0f, 1.0f);

		// 回転を元に戻す
		SameNumberRotation();
		// ベジェ位置へ
		worldTransform_.translation = Bezier2(parryP0_, parryP1_, parryP2_, t);

		// 到達
		if(t >= 1.0f) {
			worldTransform_.scale = param_.hitScale;
			isSocket_			  = true;
			parryCurveInit_		  = false;
			return;
		}
	}

	// パラメータ方向に移動
	CalyxEngine::Vector3 dire	  = param_.direction.Normalize();
	CalyxEngine::Vector3 velocity = dire * param_.speed * dt;
	worldTransform_.translation += velocity;
}
