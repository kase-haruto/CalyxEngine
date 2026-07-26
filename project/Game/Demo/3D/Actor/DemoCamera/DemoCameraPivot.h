#pragma once


#include "Engine/Foundation/Reflection/CalyxReflection.h"

#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>
#include <Engine/Foundation/Math/MathUtil.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>
#include <Engine/Scene/Reference/TransformReference.h>

/*-----------------------------------------------------------------------------------------
 * DemoCameraPivot
 * - デモ用カメラのピボットクラス
 * - メインカメラの親となり、追従機能を持つ
 *---------------------------------------------------------------------------------------*/
CALYX_PLACEABLE_OBJECT(Category = GameObject, DisplayName = "DemoCameraPivot")
/**
 * @brief DemoCameraPivotの機能を提供するクラスです。
 */
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
	void RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) override;

private:
	//==================================================================*//
	//          private functions
	//==================================================================*//
	void ApplyConfig(const SceneObjectConfig& config);
	SceneObjectConfig ExtractConfig() const;
	void ApplyCameraPivotConfig(const nlohmann::json& j);
	void ExtractCameraPivotConfig(nlohmann::json& j) const;
	const BaseTransform* ResolveTargetTransform();
	void UpdateRotationInput(float dt);
	std::shared_ptr<SceneObject> ResolveMainCamera();
	void ApplyPivotTransform(float dt);
	void ApplyCameraTransform();

	//==================================================================*//
	//          private variable
	//==================================================================*//
	CalyxEngine::TransformRef targetTransform_;     //< Inspectorで明示的に割り当てる読み取り専用Transform
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
