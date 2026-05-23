// BaseCamera.h
#pragma once

#include "ICamera.h"
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>
#include <Engine/Graphics/Buffer/CameraBuffer.h>

#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>
/* lib */
#include <numbers>

#include <d3d12.h>
#include <wrl.h>

struct CalyxEngine::Matrix4x4;
struct CalyxEngine::Vector3;

/*-----------------------------------------------------------------------------------------
 * BaseCamera
 * - カメラ基底クラス
 * - ビュー・プロジェクション行列の計算、カメラシェイク、定数バッファ管理を提供
 *---------------------------------------------------------------------------------------*/
class BaseCamera :
	public ICamera,
	public IConfigurable {
public:
	//==================================================================*//
	//			public functions
	//==================================================================*//
	BaseCamera();
	BaseCamera(const std::string& name);
	virtual ~BaseCamera() = default;

	virtual void Update(float dt) override; // 更新
	virtual void AlwaysUpdate(float dt) override;

	void         ShowImGui();    // ImGui表示
	virtual void UpdateMatrix(); // 行列の更新

	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command,
					PipelineType                                      pipelineType);
	void SetCommand(ID3D12GraphicsCommandList* command, PipelineType pipelineType);
	void SetRootCommand(ID3D12GraphicsCommandList* command, uint32_t rootIndex) const;

	void StartShake(float duration,float intensity) override; // カメラシェイク開始

	std::string_view GetObjectClassName() const override { return "BaseCamera"; }
	// config ============================================================
	void ApplyConfig();
	void ExtractConfig();
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;

	std::string GetObjectTypeName() const override { return name_; }

protected:
	//==================================================================*//
	//			protected functions
	//==================================================================*//
	CalyxEngine::Matrix4x4 MakePerspectiveFovMatrix(float fovY,float aspectRatio,float nearClip,float farClip);
	void                 SetName(const std::string& name);

public:
	//==================================================================*//
	//			getter / setter
	//==================================================================*//
	// Setter
	void SetCamera(const CalyxEngine::Vector3& pos,const CalyxEngine::Vector3& rotate);

	// Getter
	const CalyxEngine::Matrix4x4& GetViewMatrix() const;
	const CalyxEngine::Matrix4x4& GetProjectionMatrix() const;
	const CalyxEngine::Matrix4x4& GetViewProjectionMatrix() const;
	const CalyxEngine::Vector3&   GetRotate() const;
	const CalyxEngine::Vector3&   GetTranslate() const;
	float                       GetFovY() const { return fovAngleY_; }
	float                       GetAspectRatio() const { return aspectRatio_; }
	bool                        IsActive() const { return isActive_; }
	void                        SetActive(bool isActive) { isActive_ = isActive; }
	void                        SetAspectRatio(float aspect) override;
	float                       GetNear() const { return nearZ_; }

protected:
	//==================================================================*//
	//			protected variables
	//==================================================================*//

	CalyxEngine::Matrix4x4 viewMatrix_;       // ビュー行列
	CalyxEngine::Matrix4x4 projectionMatrix_; // プロジェクション行列

	float aspectRatio_ = 16.0f / 9.0f;                                          // アスペクト比
	float nearZ_       = 0.1f;                                                  // 近クリップ面
	float farZ_        = 9000.0f;                                               // 遠クリップ面
	float fovAngleY_   = 75.0f * static_cast<float>(std::numbers::pi) / 180.0f; // 垂直視野角

protected:
	// カメラシェイク関連
	bool               isShaking_      = false;
	float              shakeDuration_  = 0.0f;
	float              shakeElapsed_   = 0.0f;
	float              shakeIntensity_ = 0.0f; // シェイクの強さ
	CalyxEngine::Vector3 originalPosition_;      // シェイク前の元のカメラ位置

protected:
	//==================================================================*//
	//			protected variables
	//==================================================================*//
	bool                 isActive_ = true;      //アクティブかどうか
	CalyxEngine::Matrix4x4 viewProjectionMatrix_; // ビュープロジェクション行列

private:
	//==================================================================*//
	//			private variables
	//==================================================================*//
	Camera3DBuffer cameraBuffer_; // カメラバッファ

	ConfigurableObject<SceneObjectConfig> config_;
};
