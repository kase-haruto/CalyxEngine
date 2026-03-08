#include "FalldownGimmickActor.h"

#include "Engine/Foundation/Input/Input.h"
#include "Engine/Foundation/Utility/Converter/EnumConverter.h"
#include "Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h"
#include "Engine/Scene/Utility/SceneUtility.h"
/// ===================================================================== */
///  include space
/// ===================================================================== */

// シリアライズ可能オブジェクトとして登録
REGISTER_SCENE_OBJECT(FalldownGimmickActor)

///////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
///////////////////////////////////////////////////////////////////////////////////////////
FalldownGimmickActor::FalldownGimmickActor() {
	// コライダー初期化 (コンストラクタで行うことで、読み込み時に上書きされないようにする)
	InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_StageGimmick);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetIsDrawCollider(true);
		collider_->SetCollisionEnabled(true);
	}
}

FalldownGimmickActor::FalldownGimmickActor(const std::string& modelName, std::optional<std::string> objectName)
	: StageGimmickActor(modelName, objectName) {
	// コライダー初期化
	InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_StageGimmick);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetIsDrawCollider(true);
		collider_->SetCollisionEnabled(true);
	}
}

FalldownGimmickActor::~FalldownGimmickActor() = default;

///////////////////////////////////////////////////////////////////////////////////////////
//		初期化処理
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::Initialize() {
	// 初期トランスフォーム設定
	worldTransform_.rotation = animationStartRotation_;

	// アニメーション設定
	transformAnimation_ = std::make_unique<CalyxEngine::TransformAnimation>();
	transformAnimation_->SetTarget(&worldTransform_);
	QuaternionTransform start, end;
	start.scale		= worldTransform_.scale;
	start.rotate	= animationStartRotation_;
	start.translate = worldTransform_.translation;

	end.scale	  = worldTransform_.scale;
	end.rotate	  = animationEndRotation_;
	end.translate = worldTransform_.translation;
	transformAnimation_->SetEaseType(static_cast<CalyxEase::EaseType>(animationEaseType_));

	transformAnimation_->SetTransformStart(start);
	transformAnimation_->SetTransformEnd(end);

	// エフェクトの初期化
	falldownFx_ = SceneAPI::Instantiate<CalyxEffect::FxObject>("TrailFx");
	auto fx		= falldownFx_.lock();
	fx->LoadFromPath("Effect/EnemyBulletTrailEffect");
	fx->StopAll(); // 生成時は止めておく

	// コライダーの設定 (実行時に必要なコールバックなどの設定のみ行う)
	if(collider_) {
		collider_->SetOwner(this);
		collider_->SetOnEnter([this](Collider* other) { this->OnCollisionEnter(other); });
		collider_->SetOnStay([this](Collider* other) { this->OnCollisionStay(other); });
		collider_->SetOnExit([this](Collider* other) { this->OnCollisionExit(other); });
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
//		gui
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::DerivativeGui() {
	// トランスフォームの保存
	if(ImGui::Button("Save StartTransform")) {
		animationStartRotation_ = worldTransform_.rotation;

		QuaternionTransform start;
		start.scale		= worldTransform_.scale;
		start.rotate	= worldTransform_.rotation;
		start.translate = worldTransform_.translation;
		transformAnimation_->SetTransformStart(start);
	}

	ImGui::SameLine();
	if(ImGui::Button("Save EndTransform")) {
		animationEndRotation_ = worldTransform_.rotation;

		QuaternionTransform end;
		end.scale	  = worldTransform_.scale;
		end.rotate	  = worldTransform_.rotation;
		end.translate = worldTransform_.translation;
		transformAnimation_->SetTransformEnd(end);
	}

	ImGui::DragFloat("duration", &animationDuration_, 0.1f, 0.0f, 10.0f);

	transformAnimation_->ShowGui();

	if(transformAnimation_->EaseTypeCombo()) {
		animationEaseType_ = static_cast<int32_t>(transformAnimation_->GetEaseType());
	}

	// レール進捗について
	BackgroundActor::DerivativeGui();
}

void FalldownGimmickActor::IdleUpdate(float dt) {
	(void)dt;
	if(CalyxFoundation::Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		SetCurrentState(GimmickState::Running);
	}
}
///////////////////////////////////////////////////////////////////////////////////////////
//		トリガーされた瞬間の処理
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::OnTriggered() {
	// アニメーション再生
	transformAnimation_->Play(animationDuration_);

	// fx再生
	auto fx = falldownFx_.lock();
	fx->PlayAll();
}

///////////////////////////////////////////////////////////////////////////////////////////
//		動作中の処理
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::RunningUpdate(float dt) {
	// アニメーション更新
	transformAnimation_->Update(dt);
}

///////////////////////////////////////////////////////////////////////////////////////////
//		終了時処理
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::OnFinished() {
}

///////////////////////////////////////////////////////////////////////////////////////////
//		Serialization
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::ApplyDerivedConfigFromJson(const nlohmann::json& root, const nlohmann::json* derived) {
	(void)root;
	if(!derived) return;

	// 回転
	if(derived->contains("startRotation")) animationStartRotation_ = derived->at("startRotation").get<CalyxMath::Quaternion>();
	if(derived->contains("endRotation")) animationEndRotation_ = derived->at("endRotation").get<CalyxMath::Quaternion>();

	// その他パラメータ
	animationDuration_ = derived->value("duration", 1.0f);
	animationEaseType_ = derived->value("easeType", static_cast<int32_t>(CalyxEase::EaseType::EaseInOutSine));

	BackgroundActor::ApplyDerivedConfigFromJson(root, derived);
}

void FalldownGimmickActor::ExtractDerivedConfigToJson(nlohmann::json& root, nlohmann::json& derived) const {
	(void)root;
	derived["startRotation"] = animationStartRotation_;
	derived["endRotation"]	 = animationEndRotation_;
	derived["duration"]		 = animationDuration_;
	derived["easeType"]		 = animationEaseType_;

	BackgroundActor::ExtractDerivedConfigToJson(root, derived);
}
