#include "FollowCamera.h"
/* ========================================================================
/*		include space
/* ===================================================================== */
// engine
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Application/Input/Input.h>
#include <Engine/Application/System/System.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>

// externals
#include <externals/imgui/imgui.h>

//c++
#include <numbers>

FollowCamera::FollowCamera()
:BaseCamera(){
	BaseCamera::SetName("FollowCamera");
	transform_.rotate.x = 0.4f;
}

void FollowCamera::Update(float dt){

	//* 追従
	Adulation();

	//* 旋回
	Turning(dt);

}

void FollowCamera::AlwaysUpdate(float dt){
	BaseCamera::Update(dt);
}

Vector3 FollowCamera::CalculateOffset(){
	Vector3 result = offset;
	//カメラの角度から回転行列を計算
	Matrix4x4 matRotateX = MakeRotateXMatrix(transform_.rotate.x);
	Matrix4x4 matRotateY = MakeRotateYMatrix(transform_.rotate.y);
	Matrix4x4 matRotateZ = MakeRotateZMatrix(transform_.rotate.z);
	Matrix4x4 matRotate = Matrix4x4::Multiply(Matrix4x4::Multiply(matRotateX, matRotateY), matRotateZ);
	result = TransformNormal(result, matRotate);
	return result;
}

void FollowCamera::Turning(float dt){
	if (!target_ || isShaking_){
		return; // ターゲットが存在しない場合は処理しない
	}

	float deltaX = Input::GetRightStick().x;
	float deltaY = -Input::GetRightStick().y;

	destinationAngle_.y += deltaX * rotateSpeed * dt;

	destinationAngle_.x += deltaY * rotateSpeed * dt;

	const float maxVerticalAngle = float(std::numbers::pi) / 4.0f - 0.1f;
	destinationAngle_.x = std::clamp(destinationAngle_.x, -maxVerticalAngle, maxVerticalAngle);

	transform_.rotate.y = LerpShortAngle(transform_.rotate.y, destinationAngle_.y, 0.1f);
	transform_.rotate.x = std::lerp(transform_.rotate.x, destinationAngle_.x, 0.1f); // 🔥 X軸も補間
}


void FollowCamera::Adulation(){

	if (target_){
		//追従座標の補完
		interTarget_ = Vector3::Lerp(interTarget_, target_->translate, 0.1f);

		Vector3 cameraOffset = CalculateOffset();
		//座標をコピーしてオフセット分ずらす
		transform_.translate = interTarget_ + cameraOffset;
	}

}

void FollowCamera::UpdateMatrix(){

	// 回転行列の作成
	rotateMatrix_ = EulerToMatrix(transform_.rotate);

	// ワールド行列の初期化
	worldMatrix_ = Matrix4x4::MakeIdentity();

	// 平行移動行列の作成
	Matrix4x4 translateMatrix = MakeTranslateMatrix(transform_.translate);

	// 回転と平行移動を適用
	worldMatrix_ = Matrix4x4::Multiply(rotateMatrix_, translateMatrix);

	// ビュー行列の計算（カメラのワールド行列の逆行列）
	Matrix4x4 viewMatrix = Matrix4x4::Inverse(worldMatrix_);

	projectionMatrix_ = MakePerspectiveFovMatrix(fovAngleY_, aspectRatio_, nearZ_, farZ_);

	// ビュー行列とプロジェクション行列の掛け算
	viewProjectionMatrix_ = Matrix4x4::Multiply(viewMatrix, projectionMatrix_);

}



void FollowCamera::ShowGui(){

	//名前の表示
	SceneObject::ShowGui();

	// アクティブかどうか
	BaseCamera::ShowGui();

}