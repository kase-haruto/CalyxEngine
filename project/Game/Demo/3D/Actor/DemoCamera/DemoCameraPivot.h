#pragma once


#include "Engine/Foundation/Reflection/CalyxReflection.h"

#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>
#include <Engine/Foundation/Math/MathUtil.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>

/*-----------------------------------------------------------------------------------------
 * DemoCameraPivot
 * - デモ用カメラのピボットクラス
 * - メインカメラの親となり、追従機能を持つ
 *---------------------------------------------------------------------------------------*/
CALYX_OBJECT(Category = GameObject, DisplayName = "DemoCameraPivot")
class DemoCameraPivot final :
	public SceneObject,
	public IConfigurable {
public:
	//==================================================================*//
	//          public functions
	//==================================================================*//
	DemoCameraPivot();
	~DemoCameraPivot() override = default;

	void AlwaysUpdate(float dt) override;
	void ShowGui() override;
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;

private:
	//==================================================================*//
	//          private functions
	//==================================================================*//
	void ApplyConfig(const SceneObjectConfig& config);
	SceneObjectConfig ExtractConfig() const;
	void ApplyCameraPivotConfig(const nlohmann::json& j);
	void ExtractCameraPivotConfig(nlohmann::json& j) const;
	std::shared_ptr<SceneObject> ResolveTargetObject();
	void UpdateRotationInput(float dt);
	std::shared_ptr<SceneObject> ResolveMainCamera();
	void ApplyPivotTransform(float dt);
	void ApplyCameraTransform();

	//==================================================================*//
	//          private variable
	//==================================================================*//
	std::weak_ptr<SceneObject> targetObject_;       //< 追従対象
	std::weak_ptr<SceneObject> mainCamera_;         //< 操作対象メインカメラ
	const BaseTransform* target_ = nullptr;         //< 追従対象Transform

	bool autoFindTarget_ = true;
	CalyxEngine::Vector3 pivotLocalOffset_ = {0.0f, 1.45f, 0.0f};
	CalyxEngine::Vector3 cameraLocalOffset_ = {0.0f, 1.2f, -6.0f};
	float followSharpness_ = 18.0f;
	float yaw_ = 0.0f;
	float pitch_ = CalyxEngine::ToRadians(8.0f);
	float stickRotateSpeed_ = CalyxEngine::ToRadians(180.0f);
	float keyRotateSpeed_ = CalyxEngine::ToRadians(120.0f);
	float minPitch_ = CalyxEngine::ToRadians(-35.0f);
	float maxPitch_ = CalyxEngine::ToRadians(60.0f);
};
