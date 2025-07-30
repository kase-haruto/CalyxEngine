#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Scene/Details/SceneType.h>
#include <Engine/Graphics/RenderTarget/Detail/RenderTargetDetail.h>
// lib
#include <vector>

// forward declaration
class DxCore;
class BaseCamera;
class SceneContext;

/* ========================================================================
/* シーンインターフェース
/* ===================================================================== */
class IScene{
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	IScene();
	IScene(DxCore* dxCore);
	virtual ~IScene() = default;

	virtual void Initialize() = 0;
	virtual void Update(float dt) = 0;
	virtual void PostUpdate(ID3D12GraphicsCommandList* cmdList,
							class PipelineService* psoService) = 0;
	virtual void Draw([[maybe_unused]]ID3D12GraphicsCommandList* cmdList,
					  [[maybe_unused]] class PipelineService*,
					  [[maybe_unused]] RenderTargetType){}
	virtual void DrawSpritesOnly([[maybe_unused]] ID3D12GraphicsCommandList* cmdList,
								 [[maybe_unused]] class PipelineService* psoService) {}
	virtual void CleanUp() = 0;
	virtual void LoadAssets() = 0;
	virtual void SetTransitionRequestor(class SceneTransitionRequestor* requestor) = 0;
	//--------- accessor -----------------------------------------------------
	virtual SceneContext* GetSceneContext() const = 0;
	virtual void InjectContext([[maybe_unused]]SceneContext* ctx) {};
protected:
	//===================================================================*/
	//			protected methods
	//===================================================================*/
	DxCore* pDxCore_ = nullptr;
};
