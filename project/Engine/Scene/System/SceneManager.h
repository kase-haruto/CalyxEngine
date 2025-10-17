#pragma once
#include <Engine/scene/Base/BaseScene.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Scene/Transitioner/SceneTransitionRequestor.h>
#include <Game/Scene/Details/SceneType.h>

#include <memory>
#include <d3d12.h>
#include <unordered_map>
#include <optional>
#include <vector>

class BaseScene;
class SceneContext;
class DxCore;
class PipelineService;
class PlaySession;

class SceneManager
		: public SceneTransitionRequestor{
public:
	explicit SceneManager(DxCore* dx);
	~SceneManager();

	void Initialize();

	void Update(float dt);
	void PostUpdate(ID3D12GraphicsCommandList* cmd, PipelineService* pso);
	void Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso);
	void DrawForRenderTarget(class IRenderTarget* rt, ID3D12GraphicsCommandList* cmd, PipelineService* pso);
	void DrawNotAffectedFromPE(ID3D12GraphicsCommandList* cmd, PipelineService* pso);

	SceneContext* GetCurrentSceneContext() const;

	size_t AddScene(SceneType type, std::unique_ptr<BaseScene> scene);
	void SetCurrent(size_t index);
	size_t GetCurrentIndex() const{ return currentIdx_; }

	// 遷移要求
	void RequestSceneChange(SceneType nextScene) override;
	void BindPlaySession(PlaySession* ps) { pPlaySession_ = ps; }
	SceneContext* ActiveCtx() const;
	bool ActiveRuntimeFlag() const;
	bool GetIsEndGame()const;
	void RebindIfContextChanged();
private:
	struct SceneSlot
	{
		std::unique_ptr<BaseScene> scene;
		std::unique_ptr<SceneContext> ctx;
		bool assetsLoaded = false;
	};

	std::vector<SceneSlot> slots_;
	size_t currentIdx_ = 0;

	std::unordered_map<SceneType, size_t> typeToIndex_;
	std::optional<size_t> pendingSwitchIndex_;

	DxCore* dx_ = nullptr;
	PlaySession* pPlaySession_ = nullptr;

	SceneContext* lastBoundCtx_ = nullptr;
	uint64_t lastRuntimeGen_ = 0;
};
