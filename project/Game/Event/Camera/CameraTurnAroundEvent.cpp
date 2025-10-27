#include "CameraTurnAroundEvent.h"
/* ========================================================================
/*		include space
/* ===================================================================== */
#include "Engine/Foundation/Utility/Ease/CxEase.h"
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

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
}

/////////////////////////////////////////////////////////////////////////////////////////
//		常時更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void CameraTurnAroundEvent::AlwaysUpdate(float dt) {
	BaseEventObject::AlwaysUpdate(dt); // 行列・コライダー更新

	// カメラの向く方向をlineで表示
	Vector3 start = worldTransform_.GetWorldPosition();
	Vector3 end	  = start + direction_.Normalize() + 10.0f;
	Vector4 col(Vector3(0.518f, 0.788f, 0.545f)); // 緑色
	PrimitiveDrawer::GetInstance()->DrawLine3d(start, end, col);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		発火時処理
/////////////////////////////////////////////////////////////////////////////////////////
void CameraTurnAroundEvent::OnCollisionEnter([[maybe_unused]] Collider* other) {
	// カメラを振り向かせる
}

void CameraTurnAroundEvent::OnCollisionExit([[maybe_unused]] Collider* other) {
	// カメラを戻す
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