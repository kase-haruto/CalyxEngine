#include "BellProjectileObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine\Foundation\Utility\Ease\CxEase.h>
#include <Game\StageGimmick\Gimmicks\BellProjectile\BellProjectileTarget.h>

REGISTER_SCENE_OBJECT(BellProjectileObject)

BellProjectileObject::BellProjectileObject(const std::string&		  modelName,
										   std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void BellProjectileObject::OnCollisionEnter(Collider* other) {
	BaseGameObject* otherObj = other->GetOwner();
	if(otherObj && other->GetType() != ColliderType::Type_PlayerAttack) {
		return;
	}
	if(isParry_) {
		return;
	}
	
	// ターゲットの座標を取得する
	if (target_) {
		targetPos_ = target_->GetTargetPos();
		targetPos_.y -= target_->GetWorldTransform().scale.y * 0.8f;
	}
	
	worldTransform_.scale = param_.hitScale;
	parryStartPos_ = worldTransform_.translation;

	isParry_ = true;
	parryT_ = 0.0f;
}

void BellProjectileObject::ObjectInitialize() {
	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}

	worldTransform_.scale = param_.scale;
	worldTransform_.inheritScale = false;
	isFlying_					 = false;
	isParry_					 = false;
	isRing_ = false;
}

void BellProjectileObject::ObjectUpdate(float dt) {
	// スケールを戻す処理
	ChangeScale();

	if(isRing_) {
		return;
	}

	// 飛んでいなければ更新を飛ばす
	if(!isFlying_) {
		return;
	}

	if(isParry_) {
		parryT_ += (dt / (std::max)(0.0001f, param_.parryDuration));
		float t = std::clamp(parryT_, 0.0f, 1.0f);

		// イージング
		float easedT = CalyxEngine::ApplyEase(CalyxEngine::EaseType::EaseOutSine, t);

		// 直線移動
		worldTransform_.translation = CalyxEngine::Vector3::Lerp(parryStartPos_, targetPos_, easedT);

		// 到達
		if(t >= 1.0f) {
			if(target_) {
				target_->SetIsRing(true);
			}
			isRing_ = true;
			SetDrawEnable(false);
			if(collider_) {
				collider_->SetCollisionEnabled(false);
			}
			return;
		}
	}
	// パラメータ方向に移動
	CalyxEngine::Vector3 dire	  = param_.direction.Normalize();
	CalyxEngine::Vector3 velocity = dire * param_.speed * dt;
	worldTransform_.translation += velocity;
}
