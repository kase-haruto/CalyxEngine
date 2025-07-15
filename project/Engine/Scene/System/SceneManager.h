#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
// engine
#include <Engine/scene/Base/BaseScene.h>
#include <Engine/Application/UI/EngineUI/Core/EngineUICore.h>
#include <Engine/Graphics/RenderTarget/Detail/RenderTargetDetail.h>
#include <Engine/Scene/Controller/SceneController.h>
#include <Engine/Graphics/Device/DxCore.h>

// c++
#include <memory>
#include <array>

/* ========================================================================
/* forward
/* ===================================================================== */
class IRenderTarget;
class GraphicsSystem;

/* ========================================================================
/* クラス
/* ===================================================================== */
class SceneManager
	: public SceneTransitionRequestor{
public:
	//===================================================================*/
	//				public methods
	//===================================================================*/
	SceneManager() = default;
	SceneManager(DxCore* dxCore, GraphicsSystem* graphicsSystem);
	~SceneManager();

	void Initialize();
	void Update();
	void Draw();
	void DrawForRenderTarget(IRenderTarget* target);
	void ChangeSceneByAsset(const std::string& assetPath);

	void SetEngineUI(EngineUICore* ui);
	void RequestSceneChange(SceneType nextScene)override;
	void SetGraphicsSystem(GraphicsSystem* graphicsSystem){ pGraphicsSystem_ = graphicsSystem; }
	SceneContext* GetCurrentSceneContext() const;

	//===================================================================*/
	//				private methods
	//===================================================================*/
private:
	// シーンインスタンスの配列
	std::array<std::unique_ptr<IScene>, static_cast< int >(SceneType::count)> scenes_;

	// UIパネルなど
	EngineUICore* pEngineUI_ = nullptr;
	DxCore* pDxCore_ = nullptr;
	GraphicsSystem* pGraphicsSystem_ = nullptr;

	// 現在シーン・次シーン
	int currentSceneNo_ {static_cast< int >(SceneType::PLAY)};
	int nextSceneNo_ {static_cast< int >(SceneType::PLAY)};

	bool gameResult_ = false;
public:

	/* ▼新方式用 -----------------------------*/
	std::unique_ptr<SceneContext>        assetContext_;    // SceneAsset で生成した Context
	std::unique_ptr<SceneController>     controller_;      // 生成した Controller
	std::string                          currentAssetPath_;
	bool                                 usingAssetScene_ = false;   // どちらモードか
};
