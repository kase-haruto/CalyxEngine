#include "LuckyCatObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include "Game\StageGimmick\Gimmicks\Shoji\ShojiObject.h"

REGISTER_SCENE_OBJECT(LuckyCatObject)

LuckyCatObject::LuckyCatObject(
	const std::string& modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {

	worldTransform_.translation.y += 1.5f;
}

void LuckyCatObject::OnCollisionEnter(Collider* other) {

	// ハンマー判定かどうか確認する
	// ギミックなどへの干渉 or 相手側に追加する
	BaseGameObject* otherObj = other->GetOwner();
	if(otherObj && other->GetType() != ColliderType::Type_PlayerAttack) {
		return;
	}
	if(isParry_) {
		return;
	}
	// 障子の紙の座標を取得する
	int	 count = 0;
	bool isHit = false;
	while(!isHit) {
		shojiIndex_ = 0;
		randIndex_	= static_cast<uint32_t>(rand() % 24);
		if(randIndex_ >= 12) {
			++shojiIndex_;
			randIndex_ -= 12u;
		}
		const auto& paperObj = shojiObjs_[shojiIndex_]->GetPaperObject()[randIndex_];
		if(!paperObj->GetIsGetPosition() || count > 50) {
			isHit = true;
		}
		paperObj->SetIsGetPosition(true);
		targetPos_ = (paperObj->GetWorldTransform().translation * shojiObjs_[shojiIndex_]->GetWorldTransform().scale) +
					 shojiObjs_[shojiIndex_]->GetWorldTransform().translation;
		++count;
	}
	// 収納箱の数をプラスする
	worldTransform_.scale = param_.hitScale;
	isParry_			  = true;
}

void LuckyCatObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}

	worldTransform_.scale = CalyxEngine::Vector3::One() * param_.luckyCatScale;
	worldTransform_.inheritScale = false;
}

void LuckyCatObject::ObjectUpdate(float dt) {

	// スケールを戻す処理
	ChangeScale();

	// 障子に付いたら更新を切る
	if(isShoji_) {		
		const auto& paperObj = shojiObjs_[shojiIndex_]->GetPaperObject()[randIndex_];
		worldTransform_.translation = (paperObj->GetWorldTransform().translation * shojiObjs_[shojiIndex_]->GetWorldTransform().scale) +
					 shojiObjs_[shojiIndex_]->GetWorldTransform().translation + CalyxEngine::Vector3{0.0f, 0.0f, -0.5f};
		return;
	}

	// 飛んでいなければ更新を飛ばす
	if(!isFlying_) {
		worldTransform_.scale = CalyxEngine::Vector3::One() * param_.luckyCatScale;
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
			parryOffsetP2_ = CalyxEngine::Vector3{static_cast<float>(rand() % 3 - 1) * 0.1f, 0.25f, -0.5f};
			// 距離に応じて高さを自動調整（好みで係数調整）
			float dist		= (parryP2_ - parryP0_).Length();
			float arcHeight = std::max(0.5f, dist * 0.25f);

			parryP1_ = mid + CalyxEngine::Vector3(0.0f, arcHeight, 0.0f);
		}

		parryT_ += (dt / std::max(0.0001f, param_.parryDuration));
		float t = std::clamp(parryT_, 0.0f, 1.0f);

		// ベジェ位置へ
		auto currentPos = Bezier2(parryP0_, parryP1_, parryP2_, t);
		auto prevPos = Bezier2(parryP0_, parryP1_, parryP2_ + parryOffsetP2_, t);
		worldTransform_.translation = currentPos;
		// 回転を追加
		CalyxEngine::Vector3 velocity = currentPos - prevPos;
		CalyxEngine::Quaternion rotation = worldTransform_.rotation;
		if(velocity.Length() > 0.0001f) {
			rotation =	CalyxEngine::Quaternion::LookAt(
					currentPos,
					prevPos,
					CalyxEngine::Vector3::Up()
			);
			rotation = rotation * CalyxEngine::Quaternion::MakeRotateX(-std::numbers::pi_v<float> / 2.0f);
		}
		worldTransform_.rotation = CalyxEngine::Quaternion::Slerp(
			worldTransform_.rotation, rotation, 0.5f);

		// 到達
		if(t >= 1.0f) {
			const auto& paperObj = shojiObjs_[shojiIndex_]->GetPaperObject()[randIndex_];
			paperObj->SetModelFileNameForEditor("shojiTearPaper.obj");
			shojiObjs_[shojiIndex_]->AddClearCount();
			worldTransform_.scale = param_.hitScale;
			worldTransform_.translation = parryP2_ + CalyxEngine::Vector3{0.0f, 0.0f, -0.5f};
			isShoji_			  = true;
			parryCurveInit_		  = false;
			return;
		}
	}

	// パラメータ方向に移動
	CalyxEngine::Vector3 dire	  = param_.direction.Normalize();
	CalyxEngine::Vector3 velocity = dire * param_.speed * dt;
	// 回転を追加
	worldTransform_.translation += velocity;
}
