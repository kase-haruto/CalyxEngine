#include "FalldownGimmickActor.h"

#include "Engine/Foundation/Input/Input.h"
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

	transformAnimation_->ShowGui();
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
	AddField("startRotation", animationStartRotation_).Category("animationData").ReadOnly();
	AddField("endRotation", animationStartRotation_).Category("animationData").ReadOnly();
}

CalyxEngine::ParamPath FalldownGimmickActor::FalldownGimmickParam::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Game, "FalldownGimmickActor", "Actor/Background"};
}