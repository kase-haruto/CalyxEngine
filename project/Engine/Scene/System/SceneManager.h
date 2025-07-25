#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
// engine
#include <Engine/scene/Base/BaseScene.h>
#include <Engine/Application/UI/EngineUI/Core/EngineUICore.h>
#include <Engine/Graphics/RenderTarget/Detail/RenderTargetDetail.h>
#include <Engine/Graphics/Device/DxCore.h>

// c++
#include <memory>
#include <array>

class IRenderTarget;
class GraphicsSystem;

class SceneManager
	: public SceneTransitionRequestor{
public:
	SceneManager() = default;
	SceneManager(DxCore* dxCore,GraphicsSystem* graphicsSystem);
	~SceneManager();

	void Initialize();
	void Update();
	void Draw();
	void DrawNotAffectedFromPE();
	void DrawForRenderTarget(IRenderTarget* target);

	void SetEngineUI(EngineUICore* ui);
	void RequestSceneChange(SceneType nextScene)override;
	void SetGraphicsSystem(GraphicsSystem* graphicsSystem) { pGraphicsSystem_ = graphicsSystem; }
	SceneContext* GetCurrentSceneContext() const;


private:
	// シーンインスタンスの配列
	std::array<std::unique_ptr<IScene>, static_cast< int >(SceneType::count)> scenes_;

	// 現在シーン・次シーン
	int currentSceneNo_ {static_cast< int >(SceneType::PLAY)};
	int nextSceneNo_ {static_cast< int >(SceneType::PLAY)};

	// UIパネルなど
	EngineUICore* pEngineUI_ = nullptr;
	DxCore* pDxCore_ = nullptr;
	GraphicsSystem* pGraphicsSystem_ = nullptr;

public:
	bool gameResult_ = false;
};
