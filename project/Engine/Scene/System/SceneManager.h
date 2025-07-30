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
#include <d3d12.h>
class BaseScene;
class SceneContext;
class DxCore;
class PipelineService;

class SceneManager{
public:
	explicit SceneManager(DxCore* dx);
	~SceneManager();

	// ---------------------------------------------------------------------
	// Lifecycle
	// ---------------------------------------------------------------------
	void Initialize();

	// Tick / Render --------------------------------------------------------
	void Update(float dt);
	void PostUpdate(ID3D12GraphicsCommandList* cmd, PipelineService* pso);
	void Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso);
	void DrawForRenderTarget(IRenderTarget* rt, ID3D12GraphicsCommandList* cmd, PipelineService* pso);
	void DrawNotAffectedFromPE(ID3D12GraphicsCommandList* cmd, PipelineService* pso);

	// ---------------------------------------------------------------------
	// Scene & Context accessors
	// ---------------------------------------------------------------------
	SceneContext* GetCurrentSceneContext() const;

	size_t AddScene(std::unique_ptr<BaseScene> scene);
	void   SetCurrent(size_t index);
	size_t GetCurrentIndex() const{ return currentIdx_; }

private:
	struct SceneSlot{
		std::unique_ptr<BaseScene>   scene;
		std::unique_ptr<SceneContext> ctx;
	};

	std::vector<SceneSlot> slots_;
	size_t currentIdx_ = 0;

	DxCore* dx_ = nullptr;
};