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
FalldownGimmickActor::FalldownGimmickActor() = default;
FalldownGimmickActor::FalldownGimmickActor(const std::string& modelName, std::optional<std::string> objectName)
	: StageGimmickActor(modelName, objectName) {}
FalldownGimmickActor::~FalldownGimmickActor() = default;

///////////////////////////////////////////////////////////////////////////////////////////
//		初期化処理
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::Initialize() {
	param_.LoadParams();

	// 初期トランスフォーム設定
	worldTransform_.rotation = param_.animationStartRotation_;

	// アニメーション設定
	transformAnimation_ = std::make_unique<CalyxEngine::TransformAnimation>();
	transformAnimation_->SetTarget(&worldTransform_);
	QuaternionTransform start, end;
	start.scale		= worldTransform_.scale;
	start.rotate	= param_.animationStartRotation_;
	start.translate = worldTransform_.translation;

	end.scale	  = worldTransform_.scale;
	end.rotate	  = param_.animationEndRotation_;
	end.translate = worldTransform_.translation;
	transformAnimation_->SetEaseType(static_cast<CalyxEase::EaseType>( param_.animationEaseType_));

	transformAnimation_->SetTransformStart(start);
	transformAnimation_->SetTransformEnd(end);

	// エフェクトの初期化
	falldownFx_ = SceneAPI::Instantiate<CalyxEffect::FxObject>("TrailFx");
	auto fx		= falldownFx_.lock();
	fx->LoadFromPath("Effect/EnemyBulletTrailEffect");
	fx->StopAll(); // 生成時は止めておく

	// コライダー初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	collider_->SetType(ColliderType::Type_StageGimmick);
	collider_->SetTargetType(ColliderType::Type_Player);
	collider_->SetOwner(this);
	collider_->SetIsDrawCollider(true);
	collider_->SetCollisionEnabled(true);
}

///////////////////////////////////////////////////////////////////////////////////////////
//		gui
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::DerivativeGui() {
	// トランスフォームの保存
	if(ImGui::Button("Save StartTransform")) {
		param_.animationStartRotation_ = worldTransform_.rotation;

		QuaternionTransform start;
		start.scale		= worldTransform_.scale;
		start.rotate	= worldTransform_.rotation;
		start.translate = worldTransform_.translation;
		transformAnimation_->SetTransformStart(start);
	}

	ImGui::SameLine();
	if(ImGui::Button("Save EndTransform")) {
		param_.animationEndRotation_ = worldTransform_.rotation;

		QuaternionTransform end;
		end.scale	  = worldTransform_.scale;
		end.rotate	  = worldTransform_.rotation;
		end.translate = worldTransform_.translation;
		transformAnimation_->SetTransformEnd(end);
	}

	param_.SaveAndLoadButtonGui();

	ImGui::DragFloat("duration", &param_.animationDuration_, 0.1f, 0.0f, 10.0f);

	transformAnimation_->ShowGui();

	if(transformAnimation_->EaseTypeCombo()) {
		param_.animationEaseType_ = static_cast<int32_t>(transformAnimation_->GetEaseType());
	}
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
	transformAnimation_->Play(param_.animationDuration_);

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
//		gui
///////////////////////////////////////////////////////////////////////////////////////////
FalldownGimmickActor::FalldownGimmickParam::FalldownGimmickParam() {
	AddField("startRotation", animationStartRotation_).Category("animationData");
	AddField("endRotation", animationEndRotation_).Category("animationData");
	AddField("duration", animationDuration_).Category("animationData");
	AddField("easeType", animationEaseType_).Category("animationData");
}

CalyxEngine::ParamPath FalldownGimmickActor::FalldownGimmickParam::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Game, "FalldownGimmickActor", "Actor/Background"};
}