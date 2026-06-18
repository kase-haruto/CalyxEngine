#pragma once

#include "Engine/Foundation/Reflection/CalyxReflection.h"

#include <Engine/Objects/3D/Actor/SceneObject.h>

/*-----------------------------------------------------------------------------------------
 * DemoCameraPivot
 * - デモ用カメラのピボットクラス
 * - メインカメラの親となり、追従機能を持つ
 *---------------------------------------------------------------------------------------*/
CALYX_OBJECT(Category = GameObject, DisplayName = "DemoCameraPivot")
class DemoCameraPivot final : public SceneObject{
public:
	//==================================================================*//
	//          public functions
	//==================================================================*//
	DemoCameraPivot();
	~DemoCameraPivot() override = default;

	void ShowGui() override;

	std::string_view GetObjectClassName() const override { return "DemoCameraPivot"; }
private:
	//==================================================================*//
	//          private variable
	//==================================================================*//
	const BaseTransform* target_ = nullptr; //< 追従対象
};