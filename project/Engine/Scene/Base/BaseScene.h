#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Scene/Base/IScene.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Extensions/SkyBox/SkyBox.h>
#include <Engine/Scene/Transitioner/SceneTransitionRequestor.h>

#include <Engine/Renderer/Sprite/SpriteRenderer.h>
#include <Engine/Renderer/Model/ModelRenderer.h>
#include <Engine/Graphics/Shadow/ShadowMap/ShadowMapSystem.h>

// c++
#include "Engine/Graphics/RenderTarget/Interface/IRenderTarget.h"
#include "Engine/Graphics/Shadow/ShadowMap/GpuResource/SceneDepthResource.h"

#include <string>

/* ========================================================================
/* シーン基底クラス
/* ===================================================================== */
class BaseScene :
	public IScene{
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	BaseScene();
	~BaseScene() override = default;

	virtual void Initialize()override;
	virtual void Update([[maybe_unused]] float dt )override{}
	virtual void PostUpdate(ID3D12GraphicsCommandList* cmdList,
							class PipelineService* psoService)override;
	virtual void Draw(ID3D12GraphicsCommandList* cmdList,
					  class PipelineService* psoService,
					  IRenderTarget* rt)override;
	void DrawSpritesOnly(ID3D12GraphicsCommandList* cmdList,
					class PipelineService* psoService)override;
	void CleanUp()override{};
	virtual void LoadAssets()override{}
public:
	void SetSceneName(const std::string& name){ sceneName_ = name; }
	void InjectContext(SceneContext* ctx) override { sceneContext_ = ctx; }
	SceneContext* GetSceneContext() const { return sceneContext_; }
	void SetTransitionRequestor(CalyxScene::ISceneTransitionRequestor* requestor)override{
		transitionRequestor_ = requestor;
	}

	virtual void OnExit()override{}
	virtual void OnEnter()override{}
	virtual void OnPayload(std::unique_ptr<CalyxScene::IScenePayload> payload) {
		(void)payload;
	}
protected:
	//===================================================================*/
	//			protected methods
	//===================================================================*/
	SceneContext* sceneContext_ = nullptr;
	std::shared_ptr<SkyBox> skyBox_ = nullptr;
	std::string sceneName_ = "Scene";

	//===================================================================*/
	//			renderers
	//===================================================================*/
	std::unique_ptr<SpriteRenderer> spriteRenderer_ = nullptr;
	std::unique_ptr<ModelRenderer> modelRenderer_ = nullptr;
	std::unique_ptr<CalyxGraphics::ShadowMapSystem> shadowMapSystem_ = nullptr;

protected:
	CalyxScene::ISceneTransitionRequestor* transitionRequestor_ = nullptr;
};