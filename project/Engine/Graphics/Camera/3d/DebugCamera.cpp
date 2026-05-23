#include "DebugCamera.h"
/* ========================================================================
/*		include space
/* ===================================================================== */
// Engine
#include <Engine/Application/Settings/EngineSettings.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Utility/Func/CxUtils.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

// C++
#include "Engine/Foundation/Math/MathUtil.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {
	bool NearlyEqual(float a, float b) {
		return std::abs(a - b) < 0.0001f;
	}

	bool NearlyEqual(const CalyxEngine::Vector3& a, const CalyxEngine::Vector3& b) {
		return NearlyEqual(a.x, b.x) && NearlyEqual(a.y, b.y) && NearlyEqual(a.z, b.z);
	}

	bool NearlyEqual(const CalyxEngine::Quaternion& a, const CalyxEngine::Quaternion& b) {
		return NearlyEqual(a.x, b.x) && NearlyEqual(a.y, b.y) && NearlyEqual(a.z, b.z) && NearlyEqual(a.w, b.w);
	}

	CalyxEngine::Quaternion BuildOrbitQuaternion(const CalyxEngine::Vector2& orbitAngle) {
		CalyxEngine::Matrix4x4 matRotYaw = CalyxEngine::MakeRotateYMatrix(orbitAngle.x);
		CalyxEngine::Matrix4x4 matRotPitch = CalyxEngine::MakeRotateXMatrix(orbitAngle.y);
		CalyxEngine::Matrix4x4 matRot = CalyxEngine::Matrix4x4::Multiply(matRotPitch, matRotYaw);
		return CalyxEngine::Quaternion::FromMatrix(matRot);
	}

	CalyxEngine::Matrix4x4 BuildOrbitRotationMatrix(const CalyxEngine::Vector2& orbitAngle) {
		CalyxEngine::Matrix4x4 matRotYaw = CalyxEngine::MakeRotateYMatrix(orbitAngle.x);
		CalyxEngine::Matrix4x4 matRotPitch = CalyxEngine::MakeRotateXMatrix(orbitAngle.y);
		return CalyxEngine::Matrix4x4::Multiply(matRotPitch, matRotYaw);
	}

	CalyxEngine::Vector2 CalcOrbitAngleFromForward(const CalyxEngine::Vector3& forward) {
		if(forward.LengthSquared() <= 0.000001f) {
			return {0.0f, 0.0f};
		}
		CalyxEngine::Vector3 f = forward.Normalize();
		const float limit = static_cast<float>(std::numbers::pi) * 0.5f - 0.01f;
		float pitch = std::clamp(std::asin(-f.y), -limit, limit);
		float yaw = std::atan2(f.x, f.z);
		return {yaw, pitch};
	}
} // namespace

DebugCamera::DebugCamera(const std::string& name) {
	BaseCamera::SetName(name);
	fovAngleY_					   = static_cast<float>(std::numbers::pi) * 0.25f; // 45度
	worldTransform_.translation	   = {0.0f, 4.0f, -10.0f};
	worldTransform_.eulerRotation  = {0.0f, 0.0f, 0.0f};
	SyncOrbitFromTransform();
}

void DebugCamera::Initialize() {
	worldTransform_.rotationSource = RotationSource::Quaternion;
}
//////////////////////////////////////////////////////////////////////////////
//							メイン処理
//////////////////////////////////////////////////////////////////////////////
void DebugCamera::AlwaysUpdate(float dt) {
	if(!isActive_) {
		return;
	}

	// 入力に基づいてカメラ操作
	if(isInputEnabled_ || isDraggingRotate_ || isDraggingMove_) {
		Rotate();
		Move();
		Zoom();
	}

	if(IsTransformChangedExternally()) {
		SyncOrbitFromTransform();
	}
	ApplyOrbitToTransform();

	// BaseCameraの更新処理を呼び出す
	BaseCamera::AlwaysUpdate(dt);
}

void DebugCamera::CopyStateFrom(const DebugCamera& other) {
	ApplyState(other.CaptureState());
}

DebugCamera::State DebugCamera::CaptureState() const {
	State state;
	state.target		= target_;
	state.distance		= distance_;
	state.orbitAngle	= orbitAngle_;
	state.rotateSpeed	= rotateSpeed_;
	state.panSpeed		= panSpeed_;
	state.zoomSpeed		= zoomSpeed_;
	state.translation	= worldTransform_.translation;
	state.eulerRotation = CalyxEngine::Vector3(orbitAngle_.y, orbitAngle_.x, 0.0f);
	return state;
}

void DebugCamera::ApplyState(const State& state) {
	target_					   = state.target;
	distance_			   = state.distance;
	orbitAngle_			   = state.orbitAngle;
	rotateSpeed_		   = state.rotateSpeed;
	panSpeed_			   = state.panSpeed;
	zoomSpeed_			   = state.zoomSpeed;
	worldTransform_.translation   = state.translation;
	worldTransform_.rotation	   = BuildOrbitQuaternion(orbitAngle_);
	worldTransform_.eulerRotation = CalyxEngine::Vector3(orbitAngle_.y, orbitAngle_.x, 0.0f);
	worldTransform_.rotationSource = RotationSource::Quaternion;
	isDraggingRotate_		   = false;
	isDraggingMove_			   = false;
	StoreAppliedTransform();
}

void DebugCamera::ShowGui() {

	// 名前の表示
	SceneObject::ShowGui();

	// アクティブかどうか
	BaseCamera::ShowGui();
	worldTransform_.ShowImGui("world");

	GuiCmd::DragFloat("panSpeed", panSpeed_);
	GuiCmd::DragFloat("zoomSpeed", zoomSpeed_);
}

CalyxEngine::Vector3 DebugCamera::CalcOrbitOffset() const {
	CalyxEngine::Matrix4x4 matRot = BuildOrbitRotationMatrix(orbitAngle_);
	CalyxEngine::Vector3 offset(0.0f, 0.0f, -distance_);
	return CalyxEngine::TransformNormal(offset, matRot);
}

void DebugCamera::SyncOrbitFromTransform() {
	CalyxEngine::Quaternion currentRotation = worldTransform_.rotation;
	if(worldTransform_.rotationSource == RotationSource::Euler) {
		currentRotation = CalyxEngine::Quaternion::EulerToQuaternion(worldTransform_.eulerRotation);
	}

	CalyxEngine::Matrix4x4 rotMat = CalyxEngine::Quaternion::ToMatrix(currentRotation);
	CalyxEngine::Vector3 forward = CalyxEngine::TransformNormal(CalyxEngine::Vector3(0.0f, 0.0f, 1.0f), rotMat);
	orbitAngle_ = CalcOrbitAngleFromForward(forward);

	target_ = worldTransform_.translation - CalcOrbitOffset();
	worldTransform_.rotation = currentRotation;
	worldTransform_.rotationSource = RotationSource::Quaternion;
	StoreAppliedTransform();
}

void DebugCamera::ApplyOrbitToTransform() {
	worldTransform_.translation   = target_ + CalcOrbitOffset();
	worldTransform_.rotation	   = BuildOrbitQuaternion(orbitAngle_);
	worldTransform_.eulerRotation = CalyxEngine::Vector3(orbitAngle_.y, orbitAngle_.x, 0.0f);
	worldTransform_.rotationSource = RotationSource::Quaternion;
	StoreAppliedTransform();
}

bool DebugCamera::IsTransformChangedExternally() const {
	CalyxEngine::Quaternion currentRotation = worldTransform_.rotation;
	if(worldTransform_.rotationSource == RotationSource::Euler) {
		currentRotation = CalyxEngine::Quaternion::EulerToQuaternion(worldTransform_.eulerRotation);
	}

	return !hasAppliedTransform_ ||
			   (!NearlyEqual(worldTransform_.translation, lastAppliedTranslation_) ||
				!NearlyEqual(currentRotation, lastAppliedRotation_));
}

void DebugCamera::StoreAppliedTransform() {
	CalyxEngine::Quaternion currentRotation = worldTransform_.rotation;
	if(worldTransform_.rotationSource == RotationSource::Euler) {
		currentRotation = CalyxEngine::Quaternion::EulerToQuaternion(worldTransform_.eulerRotation);
	}

	lastAppliedTranslation_ = worldTransform_.translation;
	lastAppliedRotation_	 = currentRotation;
	hasAppliedTransform_	 = true;
}

//////////////////////////////////////////////////////////////////////////////
//							ヘルパー関数
//////////////////////////////////////////////////////////////////////////////

//*-----------------------------------------------------------------------
// 回転処理 (MMBドラッグ): ターゲット中心にカメラを回転させる
//-----------------------------------------------------------------------
void DebugCamera::Rotate() {
	bool mmbPressed = CalyxFoundation::Input::PushMouseButton(CalyxFoundation::MouseButton::Middle);
	bool shiftHeld	= CalyxFoundation::Input::PushKey(DIK_LSHIFT) || CalyxFoundation::Input::PushKey(DIK_RSHIFT);
	bool ctrlHeld	= CalyxFoundation::Input::PushKey(DIK_LCONTROL) || CalyxFoundation::Input::PushKey(DIK_RCONTROL);

	// ShiftもCtrlも押されていない -> 回転処理
	if(mmbPressed && !shiftHeld && !ctrlHeld) {
		if(!isDraggingRotate_) {
			// ドラッグ開始時に現在のマウス位置を記録
			lastMousePosRotate_ = CalyxFoundation::Input::GetMouseDelta(); // 初回のデルタは無視
			isDraggingRotate_	= true;
			return; // 初回は移動量を無視
		}

		CalyxEngine::Vector2 mouseDelta = CalyxFoundation::Input::GetMouseDelta(); // 各フレームの移動量を取得

		// マウスがほとんど動いていない場合は無視
		if(std::abs(mouseDelta.x) < 0.1f && std::abs(mouseDelta.y) < 0.1f) {
			return;
		}

		// editorの設定からカメラの回転方向を設定
		const bool isInv = CalyxEngine::EngineSettings::GetInstance()->GetData().editor.DebugCameraRotateInverse;
		const float invSign = isInv ? -1.0f : 1.0f;

		orbitAngle_.x -= mouseDelta.x * rotateSpeed_ * invSign;
		orbitAngle_.y -= mouseDelta.y * rotateSpeed_ * invSign;

		// ピッチ角度を制限 (上下90度未満)
		float limit	  = static_cast<float>(std::numbers::pi) * 0.5f - 0.01f;
		orbitAngle_.y = std::clamp(orbitAngle_.y, -limit, limit);
	} else {
		// ドラッグ終了時
		isDraggingRotate_ = false;
	}
}

//*-----------------------------------------------------------------------
// パン処理 (Shift + MMBドラッグ): カメラのターゲットを移動させる
//-----------------------------------------------------------------------
void DebugCamera::Move() {
	bool mmbPressed = CalyxFoundation::Input::PushMouseButton(CalyxFoundation::MouseButton::Middle); // 中央ボタンは通常ボタンインデックス2
	bool shiftHeld	= CalyxFoundation::Input::PushKey(DIK_LSHIFT) || CalyxFoundation::Input::PushKey(DIK_RSHIFT);
	bool ctrlHeld	= CalyxFoundation::Input::PushKey(DIK_LCONTROL) || CalyxFoundation::Input::PushKey(DIK_RCONTROL);

	// Shiftが押されていて、Ctrlは押されていない -> パン処理
	if(mmbPressed && shiftHeld && !ctrlHeld) {
		if(!isDraggingMove_) {
			// ドラッグ開始時に現在のマウス移動量を記録
			lastMousePosMove_ = CalyxFoundation::Input::GetMouseDelta(); // 初回のデルタは無視
			isDraggingMove_	  = true;
			return; // 初回は移動量を無視
		}

		CalyxEngine::Vector2 mouseDelta = CalyxFoundation::Input::GetMouseDelta(); // 各フレームの移動量を取得

		// マウスがほとんど動いていない場合は無視
		if(std::abs(mouseDelta.x) < 0.1f && std::abs(mouseDelta.y) < 0.1f) {
			return;
		}

		CalyxEngine::Matrix4x4 matRot = BuildOrbitRotationMatrix(orbitAngle_);

		// パン方向の移動量 (画面右が-X, 上が+Yになるよう調整)
		CalyxEngine::Vector3 localMove(
			-mouseDelta.x * panSpeed_,
			mouseDelta.y * panSpeed_,
			0.0f);

		// ローカル移動量をワールド座標に変換
		CalyxEngine::Vector3 worldMove = CalyxEngine::TransformNormal(localMove, matRot);

		// ターゲット位置を移動
		target_ += worldMove;
	} else {
		// ドラッグ終了時
		isDraggingMove_ = false;
	}
}

//*-----------------------------------------------------------------------
// ズーム処理 (Ctrl + MMBドラッグ or マウスホイール): カメラの距離を変更
//-----------------------------------------------------------------------
void DebugCamera::Zoom() {

	// ホイールクリックが押されている場合はズームを無視
	if(CalyxFoundation::Input::PushMouseButton(CalyxFoundation::MouseButton::Middle)) {
		return;
	}

	// マウスホイールによるズーム処理
	float wheel = CalyxFoundation::Input::GetMouseWheel(); // 1フレーム当たりのホイール回転量
	if(wheel != 0.0f) {
		distance_ -= wheel * (zoomSpeed_ * 5.0f);

		// ターゲットに近づきすぎた場合、ターゲット位置を十分前方に押し出して距離を保つ
		const float kMinDistance = 10.0f;
		if(distance_ < kMinDistance) {
			float excess = kMinDistance - distance_;
			distance_ = kMinDistance;

			CalyxEngine::Matrix4x4 matRot = BuildOrbitRotationMatrix(orbitAngle_);
			CalyxEngine::Vector3 forward = CalyxEngine::TransformNormal(CalyxEngine::Vector3(0.0f, 0.0f, 1.0f), matRot);
			target_.x += forward.x * excess;
			target_.y += forward.y * excess;
			target_.z += forward.z * excess;
		}
	}
}
REGISTER_SCENE_OBJECT(DebugCamera)