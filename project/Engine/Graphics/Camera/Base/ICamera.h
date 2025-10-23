#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Graphics/Pipeline/PipelineType.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/Transform/Transform.h>

//lib
#include <Engine/Foundation/Math/Matrix4x4.h>

//c++
#include <wrl.h>

class ICamera
	:public SceneObject{
public:
	//==================================================================*//
	//			public functions
	//==================================================================*//
	virtual~ICamera() = default;
	ICamera() = default;

	virtual void Update(float dt)override = 0;
	virtual void UpdateMatrix() = 0;
	virtual void ShowGui()override{}
	virtual void TransfarToGPU(){}  // GPUへ転送
	virtual void StartShake([[maybe_unused]] float duration, [[maybe_unused]] float intensity){};
	virtual void SetAspectRatio(float aspect) = 0;

	// config ===========================================================
};

