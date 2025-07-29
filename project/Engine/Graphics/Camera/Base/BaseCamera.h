// BaseCamera.h
#pragma once

#include "ICamera.h"
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Graphics/Buffer/CameraBuffer.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector3.h>

/* lib */
#include <numbers>

#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class BaseCamera :
	public ICamera{
public:
	//==================================================================*//
	//			public functions
	//==================================================================*//
	BaseCamera();
	BaseCamera(const std::string& name);
	virtual ~BaseCamera() = default;

	virtual void Update(float dt)override;  // 更新
	virtual void AlwaysUpdate(float dt)override;

	void ShowImGui();// ImGui表示
	virtual void UpdateMatrix();  // 行列の更新

	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command,
					PipelineType pipelineType);

	void StartShake(float duration, float intensity)override;  // カメラシェイク開始


protected:
	//==================================================================*//
	//			protected functions
	//==================================================================*//
	Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);
	void SetName(const std::string& name);


public:
	//==================================================================*//
	//			getter / setter
	//==================================================================*//
	// Setter
	void SetCamera(const Vector3& pos, const Vector3& rotate);

	// Getter
	const Matrix4x4& GetWorldMat() const{ return worldMatrix_; }
	const Matrix4x4& GetViewMatrix() const;
	const Matrix4x4& GetProjectionMatrix() const;
	const Matrix4x4& GetViewProjectionMatrix() const;
	const Vector3& GetRotate() const;
	const Vector3& GetTranslate() const;
	bool IsActive()const{ return isActive_; }
	void SetActive(bool isActive){ isActive_ = isActive; }
	void SetAspectRatio(float aspect)override;


	Matrix4x4 GetViewProjection()const{ return viewProjectionMatrix_; }

protected:
	//==================================================================*//
	//			protected variables
	//==================================================================*//

	Matrix4x4 viewMatrix_;          // ビュー行列
	Matrix4x4 projectionMatrix_;    // プロジェクション行列

	float aspectRatio_ = 16.0f / 9.0f;                           // アスペクト比
	float nearZ_ = 0.1f;                                         // 近クリップ面
	float farZ_ = 1000.0f;                                       // 遠クリップ面
	float fovAngleY_ = 90.0f * static_cast< float >(std::numbers::pi) / 180.0f;  // 垂直視野角

protected:
	// カメラシェイク関連
	bool isShaking_ = false;
	float shakeDuration_ = 0.0f;
	float shakeElapsed_ = 0.0f;
	float shakeIntensity_ = 0.0f;  // シェイクの強さ
	Vector3 originalPosition_;     // シェイク前の元のカメラ位置

protected:
	//==================================================================*//
	//			protected variables
	//==================================================================*//
	EulerTransform transform_ = {
		{1.0f, 1.0f, 1.0f},			// scale
		{0.0f, 0.0f, 0.0f},			// rotate
		{0.0f, 4.0f, -15.0f}		// translate
	};

	bool isActive_ = true;				//アクティブかどうか
	Matrix4x4 viewProjectionMatrix_;	// ビュープロジェクション行列
	Matrix4x4 worldMatrix_;				// ワールド行列

private:
	//==================================================================*//
	//			private variables
	//==================================================================*//
	Camera3DBuffer cameraBuffer_;		// カメラバッファ
};
