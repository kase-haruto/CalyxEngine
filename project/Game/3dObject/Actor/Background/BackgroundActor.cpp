#include "BackgroundActor.h"

#include "Engine/Foundation/Input/Input.h"
#include "Engine/Foundation/Utility/Converter/EnumConverter.h"
#include "Engine/Objects/Collider/BoxCollider.h"
#include "Engine/Scene/Utility/SceneUtility.h"
#include "Game/3dObject/Actor/Bullet/EnemyBullet/BaseEnemyHomingBullet.h"
#include "Game/3d/GameCamera/RailCamera.h"
#include "Game/3dObject/Actor/Bullet/EnemyBullet/BaseEnemyHomingBullet.h"

#include <Engine/Foundation/Utility/Ease/CxEase.h>
#include <algorithm>

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>

REGISTER_SCENE_OBJECT(BackgroundActor)

BackgroundActor::BackgroundActor(const std::string& modelName, std::optional<std::string> objectName)
	: Actor(modelName, objectName) {
}

BackgroundActor::BackgroundActor()	= default;
BackgroundActor::~BackgroundActor() = default;

void BackgroundActor::Initialize() {
	life_ = 3;
	fadeElapsed_ = 0.0f;
	damageFlashTimer_ = 0.0f;

	param_.LoadParams();

	// コライダーがなければ生成(デシリアライズで生成されていない場合)
	if(!collider_) {
		std::unique_ptr<BoxCollider> box = std::make_unique<BoxCollider>(true);
		box->SetName(GetName() + "BoxCollider");
		box->Initialize(CalyxMath::Vector3(1.0f));
		collider_ = std::move(box);
		collider_->SetType(ColliderType::Type_StageGimmick);
		collider_->SetTargetType(ColliderType::Type_PlayerAttack);
	}

	// コールバック設定
	if(collider_) {
		collider_->SetOnEnter([this](Collider* other) { this->OnCollisionEnter(other); });
		collider_->SetOnStay([this](Collider* other) { this->OnCollisionStay(other); });
		collider_->SetOnExit([this](Collider* other) { this->OnCollisionExit(other); });
	}

	// --- FxObject を生成して再生 ---
	hitEffect_ = SceneAPI::Instantiate<CalyxEffect::FxObject>("HitFx");
	// コンフィグ読み込み（
	auto fx = hitEffect_.lock();
	fx->LoadFromPath("Effect/BossHitEffect");
}

void BackgroundActor::Update(float dt) {

	if(damageFlashTimer_ > 0.0f) {
		damageFlashTimer_ -= dt;
		model_->SetColor(CalyxMath::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
	} else {
		model_->SetColor(CalyxMath::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	}

	// lifeが0で死亡フェード
	if(life_ <= 0 || CalyxFoundation::Input::GetInstance()->TriggerKey(DIK_Q)) {
		model_->SetBlendMode(BlendMode::ALPHA);

		fadeElapsed_ += dt;
		const float raito = param_.fadeDuration > 0.0f ? fadeElapsed_ / param_.fadeDuration : 1.0f;
		const int maxEaseIndex = static_cast<int>(CalyxEase::EaseType::Count) - 1;
		const int easeIndex = std::clamp(param_.fadeEaseType, 0, maxEaseIndex);
		const auto easeType = static_cast<CalyxEase::EaseType>(easeIndex);

		CalyxMath::Vector4 color = model_->GetColor();
		color.w = CalyxEase::EaseLerp(1.0f, 0.0f, raito, easeType);
		model_->SetColor(color);

		if(raito >= 1.0f) {
			SetIsAlive(false);
		}
	}
}

void BackgroundActor::DerivativeGui() {
	ImGui::SeparatorText("Fade Settings");
	param_.SaveAndLoadButtonGui();
	GuiCmd::DragFloat("fadeDuration", param_.fadeDuration, 0.1f, 0.0f, 10.0f);

	const int maxEaseIndex = static_cast<int>(CalyxEase::EaseType::Count) - 1;
	param_.fadeEaseType = std::clamp(param_.fadeEaseType, 0, maxEaseIndex);
	CalyxEase::EaseType type = static_cast<CalyxEase::EaseType>(param_.fadeEaseType);
	if(CalyxUtil::EnumConverter<CalyxEase::EaseType>::Combo("Ease Type", type)) {
		param_.fadeEaseType = static_cast<int32_t>(type);
	}

	ImGui::SeparatorText("Rail Settings");
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
}

void BackgroundActor::OnCollisionEnter(Collider* other) {
	// 衝突相手がプレイヤーの攻撃ならダメージを受ける
	if(other && other->GetType() == ColliderType::Type_PlayerAttack) {
		if(GetIsAlive()) {
			life_--;

			// --- 衝突位置を取得 ---
			CalyxMath::Vector3 hitPos = other->GetWorldPos();

			auto fx = hitEffect_.lock();
			// 位置設定
			fx->SetWorldPosition(hitPos);
			// 再生
			fx->PlayAll();

			if(life_ <= 0) {
				if(breakEffect_.expired()) {
					auto fxObj = SceneAPI::Instantiate<CalyxEffect::FxObject>("breakFx");
					fxObj->LoadFromPath("Effect/buildingBreakFx");
					fxObj->StopAll();
					fxObj->SetTransient(true); // シーン保存対象外にする
					breakEffect_ = fxObj;
				}
				if(auto breakFx = breakEffect_.lock()) {
					// --- 衝突位置を取得 ---
					// 再生
					breakFx->RestartAll();
				}
			}
		}
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

BackgroundActor::BackGroundActorData::BackGroundActorData() {
	AddField("fadeEaseType", fadeEaseType).Category("fade");
	AddField("fadeDuration", fadeDuration).Category("fade");
}

CalyxEngine::ParamPath BackgroundActor::BackGroundActorData::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Game,"BackgroundActor", "Actor/Background"};
}