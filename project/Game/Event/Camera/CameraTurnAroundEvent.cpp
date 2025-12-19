#include "CameraTurnAroundEvent.h"
/* ========================================================================
/*		include space
/* ===================================================================== */
#include "Engine/Foundation/Utility/Ease/CxEase.h"
#include "Game/3dObject/Actor/Player/Player.h"

#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

// cameraAction
#include <Engine/Graphics/Camera/Action/CameraTurnAroundAction.h>

REGISTER_SCENE_OBJECT(CameraTurnAroundEvent);

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
CameraTurnAroundEvent::CameraTurnAroundEvent() {
	// 操作対象カメラにメインカメラをセット
	cam_ = CameraManager::GetMain3d();
	SceneObject::SetName("CameraTurnAroundEvent", ObjectType::Event);

	config_.SetOnApplied([this](const CameraTurnAroundEventConfig&) {
		this->ApplyConfig();
	});

	config_.SetOnExtracted([this](const CameraTurnAroundEventConfig&) {
		this->ExtractConfig();
	});
}

CameraTurnAroundEvent::CameraTurnAroundEvent(const std::string& name) : CameraEventObject(name) {
	// 操作対象カメラにメインカメラをセット
	cam_ = CameraManager::GetMain3d();

	config_.SetOnApplied([this](const CameraTurnAroundEventConfig&) {
		this->ApplyConfig();
	});

	config_.SetOnExtracted([this](const CameraTurnAroundEventConfig&) {
		this->ExtractConfig();
	});
}

CameraTurnAroundEvent::~CameraTurnAroundEvent() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void CameraTurnAroundEvent::Initialize() {
	// 個別の調節パラメータ適用
	const std::string configRoot = "Event/";
	config_.LoadConfig(configRoot + GetName());

	// コライダーの色を黄色に設定
	collider_->SetColor(CxMath::Vector3(1,1,0));
}

/////////////////////////////////////////////////////////////////////////////////////////
//		常時更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void CameraTurnAroundEvent::AlwaysUpdate(float dt) {
	BaseEventObject::AlwaysUpdate(dt); // 行列・コライダー更新

	// カメラの向く方向をlineで表示
	CxMath::Vector3 start = worldTransform_.GetWorldPosition();
	CxMath::Vector3 end	  = start + direction_.Normalize() + 10.0f;
	CxMath::Vector4 col(CxMath::Vector3(0.518f, 0.788f, 0.545f)); // 緑色
	PrimitiveDrawer::GetInstance()->DrawLine3d(start, end, col);

	if(!cam_) {
		return;
	}
	// アクションの更新
	if(turnAction_) {turnAction_->Update(cam_,dt);}
	if(returnAction_) {returnAction_->Update(cam_,dt);}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		発火時処理
/////////////////////////////////////////////////////////////////////////////////////////
void CameraTurnAroundEvent::OnCollisionEnter([[maybe_unused]] Collider* other) {
	if(active_) return;
	active_ = true;

	Camera3d* cam = CameraManager::GetMain3d();
	if(!cam) return;

	// 元の向きを保存
	originalDir_ = cam->GetWorldTransform().GetForward();

	turnAction_ = std::make_unique<CameraTurnAroundAction>();
	turnAction_->SetEase(easeType_);
	turnAction_->SetTime(time_);
	turnAction_->SetDirection(direction_);
	turnAction_->Execute();
}

void CameraTurnAroundEvent::OnCollisionExit([[maybe_unused]] Collider* other) {
	// カメラを戻す
	if(!active_) return;
	active_ = false;

	Camera3d* cam = CameraManager::GetMain3d();
	if(!cam) return;

	// 元の方向に戻す
	returnAction_ = std::make_unique<CameraTurnAroundAction>();
	returnAction_->SetEase(easeType_);
	returnAction_->SetTime(time_);
	returnAction_->SetDirection(originalDir_);
	returnAction_->Execute();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		設定適用
/////////////////////////////////////////////////////////////////////////////////////////
void CameraTurnAroundEvent::ApplyConfig() {
	CameraEventObject::ApplyConfig();

	const CameraTurnAroundEventConfig& cfg = config_.GetConfig();
	time_								   = cfg.time;
	easeType_							   = static_cast<Cx::Ease::EaseType>(cfg.easeType);
	direction_							   = cfg.direction;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		設定掃き出し
/////////////////////////////////////////////////////////////////////////////////////////
void CameraTurnAroundEvent::ExtractConfig() {
	CameraEventObject::ExtractConfig();

	CameraTurnAroundEventConfig& cfg = config_.GetConfig();
	cfg.time						 = time_;
	cfg.easeType					 = static_cast<int16_t>(easeType_);
	cfg.direction					 = direction_;
}

void CameraTurnAroundEvent::DerivativeGui() {
	if(ImGui::CollapsingHeader("Turn Around Parm")) {
		GuiCmd::DragFloat3("dir", direction_);
		GuiCmd::DragFloat("time", time_);
		// Cx::Ease::SelectEase(easeType_);
	}
}
void CameraTurnAroundEvent::ConfigGUi() {
	config_.ShowGui("Event/" + GetName());
}