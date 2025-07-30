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

class SceneManager : public SceneTransitionRequestor {
public:
	explicit SceneManager(DxCore* dxCore);
	~SceneManager();

	void Initialize();
	void Update(float dt);
	void PostUpdate(ID3D12GraphicsCommandList* cmd, PipelineService* pso);
	void Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso);
	void DrawNotAffectedFromPE(ID3D12GraphicsCommandList* cmd, PipelineService* pso);

	void SetEngineUI(EngineUICore* ui) { pEngineUI_ = ui; }
	void RequestSceneChange(SceneType next) override;

	SceneContext* GetCurrentSceneContext() const;

private:
	struct SceneSlot {
		std::unique_ptr<IScene>       scene;
		std::unique_ptr<SceneContext> ctx;
	};
	std::array<SceneSlot, static_cast<int>(SceneType::count)> slots_{};

	int currentSceneNo_{ static_cast<int>(SceneType::PLAY) };
	int nextSceneNo_{ static_cast<int>(SceneType::PLAY) };

	EngineUICore* pEngineUI_ = nullptr;
	DxCore* pDxCore_ = nullptr;

	/* helpers */
	void SwitchScene(int newNo);
	void DrawForRenderTarget(IRenderTarget* rt,
							 ID3D12GraphicsCommandList* cmd,
							 PipelineService* pso);
};