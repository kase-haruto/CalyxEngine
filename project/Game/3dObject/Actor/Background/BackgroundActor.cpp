#include "BackgroundActor.h"

#include "Engine/Objects/Collider/BoxCollider.h"
#include "Game/3dObject/Actor/Bullet/EnemyBullet/BaseEnemyHomingBullet.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Game/3d/GameCamera/RailCamera.h>

REGISTER_SCENE_OBJECT(BackgroundActor)

BackgroundActor::BackgroundActor(const std::string& modelName, std::optional<std::string> objectName)
	: Actor(modelName, objectName) {
	life_ = 3;

	// 衝突の設定(boxで初期化
	std::unique_ptr<BoxCollider> box = std::make_unique<BoxCollider>(true);
	box->SetName(GetName() + "BoxCollider");   //< コライダー名前設定
	box->Initialize(CalyxMath::Vector3(1.0f)); //< サイズ設定
	collider_ = std::move(box);
	collider_->SetType(ColliderType::Type_StageGimmick);
	collider_->SetTargetType(ColliderType::Type_PlayerAttack);

	collider_->SetOnEnter([this](Collider* other) { this->OnCollisionEnter(other); });
	collider_->SetOnStay([this](Collider* other) { this->OnCollisionStay(other); });
	collider_->SetOnExit([this](Collider* other) { this->OnCollisionExit(other); });
}

BackgroundActor::BackgroundActor()	= default;
BackgroundActor::~BackgroundActor() = default;

void BackgroundActor::Update(float dt) {
	// lifeが０で死亡
	if(life_ <= 0) {
		model_->SetBlendMode(BlendMode::ALPHA);
		// 志望処理（αを減らす）
		// 完全に透明になったら非表示にする
		CalyxMath::Vector4 color = model_->GetColor();
		color.w -= dt; // αを減らす
		if(color.w <= 0) {
			color.w = 0;
			SetIsAlive(false); // 非表示にする
		}
		model_->SetColor(color); // 減衰したαを反映
	}
}

void BackgroundActor::DerivativeGui() {
#if defined(_DEBUG) || defined(DEVELOP)
	ImGui::Checkbox("Stop Rail", &isStopRail_);
	if(isStopRail_) {
		ImGui::DragFloat("Stop Offset", &stopOffset_, 0.1f, 0.0f, 300.0f);
		ImGui::Checkbox("Auto Progress", &autoCalculateProgress_);
		if(autoCalculateProgress_) {
			if(ImGui::Button("Sync Rail Progress")) {
				auto* ctx = SceneContext::Current();
				if(ctx) {
					auto railCam = ctx->FindFirst<RailCamera>();
					if(railCam) {
						// 最近接点からオフセット分を引いて停止位置とする
						float nearestS = railCam->GetSplineData().FindNearestDistance(GetWorldPosition());
						stopProgress_  = (std::max)(0.0f, nearestS - stopOffset_);
					}
				}
			}
			ImGui::Text("Auto Stop Progress: %.2f (Base-Offset)", stopProgress_);
		} else {
			ImGui::DragFloat("Stop Progress", &stopProgress_, 0.1f, 0.0f, 10000.0f);
		}
	}
#endif
}

void BackgroundActor::OnCollisionEnter(Collider* other) {
	// 衝突相手がプレイヤーの攻撃ならダメージを受ける
	if(other && other->GetType() == ColliderType::Type_PlayerAttack) {
		life_--;

	}
}
void BackgroundActor::ApplyDerivedConfigFromJson(const nlohmann::json& root, const nlohmann::json* derived) {
	(void)root;
	if(!derived) return;

	isStopRail_			   = derived->value("isStopRail", false);
	stopProgress_		   = derived->value("stopProgress", 0.0f);
	stopOffset_			   = derived->value("stopOffset", 2.0f);
	autoCalculateProgress_ = derived->value("autoCalculateProgress", true);
}

void BackgroundActor::ExtractDerivedConfigToJson(nlohmann::json& root, nlohmann::json& derived) const {
	(void)root;
	derived["isStopRail"]			 = isStopRail_;
	derived["stopProgress"]			 = stopProgress_;
	derived["stopOffset"]			 = stopOffset_;
	derived["autoCalculateProgress"] = autoCalculateProgress_;
}