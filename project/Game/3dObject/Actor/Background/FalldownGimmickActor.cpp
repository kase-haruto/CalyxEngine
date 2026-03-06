#include "FalldownGimmickActor.h"

#include "Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h"
#include "Engine/Scene/Utility/SceneUtility.h"
/// ===================================================================== */
///  include space
/// ===================================================================== */

REGISTER_SCENE_OBJECT(FalldownGimmickActor)

///////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
///////////////////////////////////////////////////////////////////////////////////////////

FalldownGimmickActor::FalldownGimmickActor() = default;
FalldownGimmickActor::FalldownGimmickActor(const std::string& modelName, std::optional<std::string> objectName)
	: StageGimmickActor(modelName, objectName) {}
FalldownGimmickActor::~FalldownGimmickActor() = default;

void FalldownGimmickActor::Initialize() {

	// アニメーション設定
	transformAnimation_ = std::make_unique<CalyxEngine::TransformAnimation>();
	transformAnimation_->SetTarget(&worldTransform_);

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
		transformAnimation_->SetTransformStart(worldTransform_);
	}
	ImGui::SameLine();
	if(ImGui::Button("Save EndTransform")) {
		transformAnimation_->SetTransformEnd(worldTransform_);
	}

	transformAnimation_->ShowGui();
}

///////////////////////////////////////////////////////////////////////////////////////////
//		gui
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::OnTriggered() {
	// fx再生
	auto fx = falldownFx_.lock();
	fx->PlayAll();

}

///////////////////////////////////////////////////////////////////////////////////////////
//		gui
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::RunningUpdate(float dt) {
	// アニメーション更新
	transformAnimation_->Update(dt);
}

///////////////////////////////////////////////////////////////////////////////////////////
//		gui
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::OnFinished() {
}