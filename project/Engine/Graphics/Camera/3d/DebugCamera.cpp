#include "DebugCamera.h"
/* ========================================================================
/*		include space
/* ===================================================================== */
// Engine
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine/Foundation/Utility/Func/CxUtils.h>

// C++
#include <algorithm>
#include <numbers>

DebugCamera::DebugCamera(const std::string& name){
	BaseCamera::SetName(name);
	fovAngleY_ = static_cast< float >(std::numbers::pi) * 0.25f; // 45度
	worldTransform_.translation = {0.0f, 4.0f, -10.0f};
}

//////////////////////////////////////////////////////////////////////////////
//							メイン処理
//////////////////////////////////////////////////////////////////////////////
void DebugCamera::AlwaysUpdate(float dt){
	if (!isActive_){ return; }

	// 入力に基づいてカメラ操作
	if (isInputEnabled_ || isDraggingRotate_ || isDraggingMove_) {
		Rotate();
		Move();
		Zoom();
	}

	// カメラの姿勢・行列を更新
	{
		// orbitAngle_.x = 水平方向(Yaw)
		// orbitAngle_.y = 垂直方向(Pitch)
		// 距離と角度からカメラの相対座標を計算
		CalyxMath::Matrix4x4 matRotYaw =CalyxMath::MakeRotateYMatrix(orbitAngle_.x);
		CalyxMath::Matrix4x4 matRotPitch = CalyxMath::MakeRotateXMatrix(orbitAngle_.y);
		CalyxMath::Matrix4x4 matRot = CalyxMath::Matrix4x4::Multiply(matRotPitch, matRotYaw);

		// Z方向に距離分だけオフセットし、回転行列を適用
		CalyxMath::Vector3 offset(0.0f, 0.0f, -distance_);
		offset = CalyxMath::TransformNormal(offset, matRot);

		// カメラの位置 = ターゲット + オフセット
		worldTransform_.translation = target_ + offset;

		// カメラの回転は (Pitch, Yaw, 0)
		worldTransform_.eulerRotation = CalyxMath::Vector3(orbitAngle_.y, orbitAngle_.x, 0.0f);
	}

	// BaseCameraの更新処理を呼び出す
	BaseCamera::AlwaysUpdate(dt);
}

void DebugCamera::ShowGui(){

	//名前の表示
	SceneObject::ShowGui();

	// アクティブかどうか
	BaseCamera::ShowGui();

	GuiCmd::DragFloat("panSpeed", panSpeed_);
	GuiCmd::DragFloat("zoomSpeed", zoomSpeed_);
}

//////////////////////////////////////////////////////////////////////////////
//							ヘルパー関数
//////////////////////////////////////////////////////////////////////////////

//*-----------------------------------------------------------------------
// 回転処理 (MMBドラッグ): ターゲット中心にカメラを回転させる
//-----------------------------------------------------------------------
void DebugCamera::Rotate(){
	bool mmbPressed = CalyxFoundation::Input::PushMouseButton(CalyxFoundation::MouseButton::Middle);
	bool shiftHeld = CalyxFoundation::Input::PushKey(DIK_LSHIFT) || CalyxFoundation::Input::PushKey(DIK_RSHIFT);
	bool ctrlHeld = CalyxFoundation::Input::PushKey(DIK_LCONTROL) || CalyxFoundation::Input::PushKey(DIK_RCONTROL);

	// ShiftもCtrlも押されていない -> 回転処理
	if (mmbPressed && !shiftHeld && !ctrlHeld){
		if (!isDraggingRotate_){
			// ドラッグ開始時に現在のマウス位置を記録
			lastMousePosRotate_ = CalyxFoundation::Input::GetMouseDelta(); // 初回のデルタは無視
			isDraggingRotate_ = true;
			return; // 初回は移動量を無視
		}

		CalyxMath::Vector2 mouseDelta = CalyxFoundation::Input::GetMouseDelta(); // 各フレームの移動量を取得

		// マウスがほとんど動いていない場合は無視
		if (std::abs(mouseDelta.x) < 0.1f && std::abs(mouseDelta.y) < 0.1f){
			return;
		}

		// 回転処理
		orbitAngle_.x -= mouseDelta.x * rotateSpeed_;
		orbitAngle_.y -= mouseDelta.y * rotateSpeed_;

		// ピッチ角度を制限 (上下90度未満)
		float limit = static_cast< float >(std::numbers::pi) * 0.5f - 0.01f;
		orbitAngle_.y = std::clamp(orbitAngle_.y, -limit, limit);
	} else{
		// ドラッグ終了時
		isDraggingRotate_ = false;
	}
}

//*-----------------------------------------------------------------------
// パン処理 (Shift + MMBドラッグ): カメラのターゲットを移動させる
//-----------------------------------------------------------------------
void DebugCamera::Move(){
	bool mmbPressed = CalyxFoundation::Input::PushMouseButton(CalyxFoundation::MouseButton::Middle); // 中央ボタンは通常ボタンインデックス2
	bool shiftHeld = CalyxFoundation::Input::PushKey(DIK_LSHIFT) || CalyxFoundation::Input::PushKey(DIK_RSHIFT);
	bool ctrlHeld = CalyxFoundation::Input::PushKey(DIK_LCONTROL) || CalyxFoundation::Input::PushKey(DIK_RCONTROL);

	// Shiftが押されていて、Ctrlは押されていない -> パン処理
	if (mmbPressed && shiftHeld && !ctrlHeld){
		if (!isDraggingMove_){
			// ドラッグ開始時に現在のマウス移動量を記録
			lastMousePosMove_ = CalyxFoundation::Input::GetMouseDelta(); // 初回のデルタは無視
			isDraggingMove_ = true;
			return; // 初回は移動量を無視
		}

		CalyxMath::Vector2 mouseDelta = CalyxFoundation::Input::GetMouseDelta(); // 各フレームの移動量を取得

		// マウスがほとんど動いていない場合は無視
		if (std::abs(mouseDelta.x) < 0.1f && std::abs(mouseDelta.y) < 0.1f){
			return;
		}

		// カメラの回転行列を作成
		CalyxMath::Matrix4x4 matRotYaw = CalyxMath::MakeRotateYMatrix(orbitAngle_.x);
		CalyxMath::Matrix4x4 matRotPitch = CalyxMath::MakeRotateXMatrix(orbitAngle_.y);
		CalyxMath::Matrix4x4 matRot = CalyxMath::Matrix4x4::Multiply(matRotPitch, matRotYaw);

		// パン方向の移動量 (画面右が-X, 上が+Yになるよう調整)
		CalyxMath::Vector3 localMove(
			-mouseDelta.x * panSpeed_,
			mouseDelta.y * panSpeed_,
			0.0f
		);

		// ローカル移動量をワールド座標に変換
		CalyxMath::Vector3 worldMove = CalyxMath::TransformNormal(localMove, matRot);

		// ターゲット位置を移動
		target_ += worldMove;
	} else{
		// ドラッグ終了時
		isDraggingMove_ = false;
	}
}

//*-----------------------------------------------------------------------
// ズーム処理 (Ctrl + MMBドラッグ or マウスホイール): カメラの距離を変更
//-----------------------------------------------------------------------
void DebugCamera::Zoom(){

	// ホイールクリックが押されている場合はズームを無視
	if(CalyxFoundation::Input::PushMouseButton(CalyxFoundation::MouseButton::Middle)) {
		return;
	}

	// マウスホイールによるズーム処理
	float wheel = CalyxFoundation::Input::GetMouseWheel(); // 1フレーム当たりのホイール回転量
	if (wheel != 0.0f){
		distance_ -= wheel * (zoomSpeed_ * 5.0f);
		distance_ = ( std::max ) (0.01f, distance_);
	}
}
REGISTER_SCENE_OBJECT(DebugCamera)