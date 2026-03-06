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

	// エフェクトの初期化
	falldownFx_ = SceneAPI::Instantiate<CalyxEffect::FxObject>("TrailFx");
	auto fx		= falldownFx_.lock();
	fx->LoadFromPath("Effect/EnemyBulletTrailEffect");
	fx->StopAll(); // 生成時は止めておく
}

///////////////////////////////////////////////////////////////////////////////////////////
//		gui
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::DerivativeGui() {
	// 倒れこむアニメーションのgui
	falldownAnimation_.ImGui("falldownAnimation");
}

///////////////////////////////////////////////////////////////////////////////////////////
//		gui
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::OnTriggered() {
	// アニメーション再生
	falldownAnimation_.Start();
	// fx再生
	auto fx = falldownFx_.lock();
	fx->PlayAll();
}

///////////////////////////////////////////////////////////////////////////////////////////
//		gui
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::RunningUpdate(float dt) {
	// アニメーション適用
	falldownAnimation_.LerpValue(worldTransform_.rotation, dt);
}

///////////////////////////////////////////////////////////////////////////////////////////
//		gui
///////////////////////////////////////////////////////////////////////////////////////////
void FalldownGimmickActor::OnFinished() {
	falldownAnimation_.Stop();
}