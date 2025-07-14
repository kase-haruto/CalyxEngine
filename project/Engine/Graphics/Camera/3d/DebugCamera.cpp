#include "DebugCamera.h"
/* ========================================================================
/*		include space
/* ===================================================================== */
// Engine
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Application/Input/Input.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>

// externals
#include <externals/imgui/imgui.h>

// C++
#include <algorithm>  // std::clamp
#include <numbers>    // std::numbers::pi

DebugCamera::DebugCamera()
	: BaseCamera(),
	lastMousePosRotate_ {0.0f, 0.0f},
	isDraggingRotate_ {false},
	lastMousePosMove_ {0.0f, 0.0f},
	isDraggingMove_ {false}{
	BaseCamera::SetName("DebugCamera");
	fovAngleY_ = static_cast< float >(std::numbers::pi) * 0.25f; // 45度

	transform_.translate = {0.0f, 4.0f, -10.0f};
}

//////////////////////////////////////////////////////////////////////////////
//							メイン処理
//////////////////////////////////////////////////////////////////////////////
void DebugCamera::Update(){
	if (!isActive_){ return; }

	// 入力に基づいてカメラ操作
	Rotate();
	Move();
	Zoom();

	// カメラの姿勢・行列を更新
	{
		// orbitAngle_.x = 水平方向(Yaw)
		// orbitAngle_.y = 垂直方向(Pitch)
		// 距離と角度からカメラの相対座標を計算
		Matrix4x4 matRotYaw = MakeRotateYMatrix(orbitAngle_.x);
		Matrix4x4 matRotPitch = MakeRotateXMatrix(orbitAngle_.y);
		Matrix4x4 matRot = Matrix4x4::Multiply(matRotPitch, matRotYaw);

		// Z方向に距離分だけオフセットし、回転行列を適用
		Vector3 offset(0.0f, 0.0f, -distance_);
		offset = TransformNormal(offset, matRot);

		// カメラの位置 = ターゲット + オフセット
		transform_.translate = target_ + offset;

		// カメラの回転は (Pitch, Yaw, 0)
		transform_.rotate = Vector3(orbitAngle_.y, orbitAngle_.x, 0.0f);
	}

	// BaseCameraの更新処理を呼び出す
	BaseCamera::Update();
}

void DebugCamera::ShowGui(){

	//名前の表示
	SceneObject::ShowGui();

	// アクティブかどうか
	BaseCamera::ShowGui();
}

//////////////////////////////////////////////////////////////////////////////
//							ヘルパー関数
//////////////////////////////////////////////////////////////////////////////

//*-----------------------------------------------------------------------
// 回転処理 (MMBドラッグ): ターゲット中心にカメラを回転させる
//-----------------------------------------------------------------------
void DebugCamera::Rotate(){
	bool mmbPressed = Input::PushMouseButton(MouseButton::Middle);
	bool shiftHeld = Input::PushKey(DIK_LSHIFT) || Input::PushKey(DIK_RSHIFT);
	bool ctrlHeld = Input::PushKey(DIK_LCONTROL) || Input::PushKey(DIK_RCONTROL);

	// ShiftもCtrlも押されていない -> 回転処理
	if (mmbPressed && !shiftHeld && !ctrlHeld){
		if (!isDraggingRotate_){
			// ドラッグ開始時に現在のマウス位置を記録
			lastMousePosRotate_ = Input::GetMouseDelta(); // 初回のデルタは無視
			isDraggingRotate_ = true;
			return; // 初回は移動量を無視
		}

		Vector2 mouseDelta = Input::GetMouseDelta(); // 各フレームの移動量を取得

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
	bool mmbPressed = Input::PushMouseButton(MouseButton::Middle); // 中央ボタンは通常ボタンインデックス2
	bool shiftHeld = Input::PushKey(DIK_LSHIFT) || Input::PushKey(DIK_RSHIFT);
	bool ctrlHeld = Input::PushKey(DIK_LCONTROL) || Input::PushKey(DIK_RCONTROL);

	// Shiftが押されていて、Ctrlは押されていない -> パン処理
	if (mmbPressed && shiftHeld && !ctrlHeld){
		if (!isDraggingMove_){
			// ドラッグ開始時に現在のマウス移動量を記録
			lastMousePosMove_ = Input::GetMouseDelta(); // 初回のデルタは無視
			isDraggingMove_ = true;
			return; // 初回は移動量を無視
		}

		Vector2 mouseDelta = Input::GetMouseDelta(); // 各フレームの移動量を取得

		// マウスがほとんど動いていない場合は無視
		if (std::abs(mouseDelta.x) < 0.1f && std::abs(mouseDelta.y) < 0.1f){
			return;
		}

		// カメラの回転行列を作成
		Matrix4x4 matRotYaw = MakeRotateYMatrix(orbitAngle_.x);
		Matrix4x4 matRotPitch = MakeRotateXMatrix(orbitAngle_.y);
		Matrix4x4 matRot = Matrix4x4::Multiply(matRotPitch, matRotYaw);

		// パン方向の移動量 (画面右が-X, 上が+Yになるよう調整)
		Vector3 localMove(
			-mouseDelta.x * panSpeed_,
			mouseDelta.y * panSpeed_,
			0.0f
		);

		// ローカル移動量をワールド座標に変換
		Vector3 worldMove = TransformNormal(localMove, matRot);

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
	if (Input::PushMouseButton(MouseButton::Middle)){
		return;
	}

	// マウスホイールによるズーム処理
	float wheel = Input::GetMouseWheel(); // 1フレーム当たりのホイール回転量
	if (wheel != 0.0f){
		distance_ -= wheel * (zoomSpeed_ * 5.0f);
		distance_ = (std::max)(0.01f, distance_);
	}
}
