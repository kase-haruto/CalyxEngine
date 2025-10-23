#include "BaseCamera.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
// lib
#include <Engine/Foundation/Utility/Func/MyFunc.h>

// c++
#include <cmath>

/////////////////////////////////////////////////////////////////////////
//  コンストラクタ
/////////////////////////////////////////////////////////////////////////
BaseCamera::BaseCamera()
	:viewMatrix_(Matrix4x4::Inverse(worldTransform_.matrix.world)),
	projectionMatrix_(MakePerspectiveFovMatrix(fovAngleY_, aspectRatio_, nearZ_, farZ_)){
	viewProjectionMatrix_ = Matrix4x4::Multiply(viewMatrix_, projectionMatrix_);

	/* バッファの生成とマッピング =======================*/
	cameraBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice().Get());
	worldTransform_.rotationSource = RotationSource::Euler;

	isEnableRaycast_ = false;
}

BaseCamera::BaseCamera(const std::string& name)
	:viewMatrix_(Matrix4x4::Inverse(worldTransform_.matrix.world)),
	projectionMatrix_(MakePerspectiveFovMatrix(fovAngleY_, aspectRatio_, nearZ_, farZ_)){
	SceneObject::SetName(name, ObjectType::Camera);

	viewProjectionMatrix_ = Matrix4x4::Multiply(viewMatrix_, projectionMatrix_);
	/* バッファの生成とマッピング =======================*/
	cameraBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice().Get());
	worldTransform_.rotationSource = RotationSource::Euler;

	isEnableRaycast_=false;
}

/////////////////////////////////////////////////////////////////////////
//  更新
/////////////////////////////////////////////////////////////////////////
void BaseCamera::Update(float dt){
	// シェイク処理
	if (isShaking_){
		shakeElapsed_ += dt;
		if (shakeElapsed_ < shakeDuration_){
			float offsetX = ((rand() / ( float ) RAND_MAX) * 2.0f - 1.0f) * shakeIntensity_;
			float offsetY = ((rand() / ( float ) RAND_MAX) * 2.0f - 1.0f) * shakeIntensity_;
			float offsetZ = ((rand() / ( float ) RAND_MAX) * 2.0f - 1.0f) * shakeIntensity_;

			// 現在のカメラ位置にオフセットを加算
			worldTransform_.translation = originalPosition_ + Vector3(offsetX, offsetY, offsetZ);
		} else{
			// シェイク終了時に元の位置に戻す
			isShaking_ = false;
			worldTransform_.translation = originalPosition_;
		}
	}

}

void BaseCamera::AlwaysUpdate([[maybe_unused]]float dt){
	worldTransform_.Update();
	UpdateMatrix();
}

/////////////////////////////////////////////////////////////////////////
//  行列の更新
/////////////////////////////////////////////////////////////////////////
void BaseCamera::UpdateMatrix(){
	// 行列の更新
	viewMatrix_ = Matrix4x4::Inverse(worldTransform_.matrix.world);
	projectionMatrix_ = MakePerspectiveFovMatrix(fovAngleY_, aspectRatio_, nearZ_, farZ_);
	viewProjectionMatrix_ = Matrix4x4::Multiply(viewMatrix_, projectionMatrix_);

	cameraBuffer_.Update(viewMatrix_, projectionMatrix_, worldTransform_.translation);
}

/////////////////////////////////////////////////////////////////////////
//  projection行列の作成
/////////////////////////////////////////////////////////////////////////
Matrix4x4 BaseCamera::MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip){
	Matrix4x4 result = {
		1 / (aspectRatio * std::tan(fovY / 2)), 0, 0, 0,
		0, 1 / std::tan(fovY / 2), 0, 0,
		0, 0, farClip / (farClip - nearClip), 1,
		0, 0, -nearClip * farClip / (farClip - nearClip), 0
	};
	return result;
}

/////////////////////////////////////////////////////////////////////////
//  カメラシェイク開始
/////////////////////////////////////////////////////////////////////////
void BaseCamera::StartShake(float duration, float intensity){
	if (!isShaking_){
		originalPosition_ = worldTransform_.translation;  // 現在の位置を記憶
	}
	isShaking_ = true;
	shakeDuration_ = duration;
	shakeElapsed_ = 0.0f;
	shakeIntensity_ = intensity;
}

void BaseCamera::ApplyConfig() {
	const auto& cfg = config_.GetConfig();

	name_ = cfg.name;
	id_ = cfg.guid;
	parentId_ = cfg.parentGuid;
}

void BaseCamera::ExtractConfig() {
	auto& cfg = config_.GetConfig();
	cfg.objectType = static_cast<int>(objectType_);
	cfg.name = name_;
	cfg.guid = id_;
	cfg.parentGuid = parentId_;
}

void BaseCamera::ApplyConfigFromJson([[maybe_unused]]const nlohmann::json& j) {
	config_.ApplyConfigFromJson(j);
	ApplyConfig();
}

void BaseCamera::ExtractConfigToJson([[maybe_unused]] nlohmann::json& j) const {
	const_cast<BaseCamera*>(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);
}

/////////////////////////////////////////////////////////////////////////
//  アクセッサ
/////////////////////////////////////////////////////////////////////////
#pragma region アクセッサ
void BaseCamera::SetName(const std::string& name){
	SceneObject::SetName(name, ObjectType::Camera);
}

void BaseCamera::SetCamera(const Vector3& pos, const Vector3& rotate){
	worldTransform_.translation = pos;
	worldTransform_.eulerRotation = rotate;
}

const Matrix4x4& BaseCamera::GetViewMatrix() const{
	return viewMatrix_;
}

const Matrix4x4& BaseCamera::GetProjectionMatrix() const{
	return projectionMatrix_;
}

const Matrix4x4& BaseCamera::GetViewProjectionMatrix() const{
	return viewProjectionMatrix_;
}

const Vector3& BaseCamera::GetRotate() const{
	return worldTransform_.eulerRotation;
}

const Vector3& BaseCamera::GetTranslate() const{
	return worldTransform_.translation;
}

#pragma endregion

void BaseCamera::SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command, PipelineType pipelineType){
	cameraBuffer_.SetCommand(command.Get(), pipelineType);
}

void BaseCamera::SetAspectRatio(float aspect){
	aspectRatio_ = aspect;

	float adjustedFov = fovAngleY_;

	// 画面が極端に狭い・広い場合はFOVを補正する
	const float lowAspectThreshold = 0.6f;
	const float highAspectThreshold = 2.0f;

	if (aspect < lowAspectThreshold){
		adjustedFov *= 1.0f + (lowAspectThreshold - aspect); // 縦長 → 視野を広げる
	} else if (aspect > highAspectThreshold){
		adjustedFov *= 1.0f + (aspect - highAspectThreshold) * 0.5f; // 横長 → 少し広げる
	}

	projectionMatrix_ = Matrix4x4::PerspectiveFovRH(adjustedFov, aspect, nearZ_, farZ_);
}

REGISTER_SCENE_OBJECT(BaseCamera)