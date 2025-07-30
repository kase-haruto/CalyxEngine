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
#include <Engine/Application/System/PlaySession.h>

// c++
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
					  RenderTargetType renderTargetType)override;
	void DrawSpritesOnly(ID3D12GraphicsCommandList* cmdList,
					class PipelineService* psoService)override;
	void CleanUp()override{};
	virtual void LoadAssets()override{}
public:
	void SetSceneName(const std::string& name){ sceneName_ = name; }
	void InjectContext(SceneContext* ctx) override { sceneContext_ = ctx; }
	SceneContext* GetSceneContext() const { return sceneContext_; }
	void SetTransitionRequestor(SceneTransitionRequestor* requestor)override{
		transitionRequestor_ = requestor;
	}
	SceneContext* ActiveCtx() const { return playSession_.GetContext(); }
protected:
	//===================================================================*/
	//			protected methods
	//===================================================================*/
	SceneContext* sceneContext_ = nullptr;
	std::shared_ptr<SkyBox> skyBox_ = nullptr;
	std::string sceneName_ = "Scene";
	PlaySession playSession_;

	//===================================================================*/
	//			renderers
	//===================================================================*/
	std::unique_ptr<SpriteRenderer> spriteRenderer_ = nullptr;
	std::unique_ptr<ModelRenderer> modelRenderer_ = nullptr;

protected:
	SceneTransitionRequestor* transitionRequestor_ = nullptr;
};
