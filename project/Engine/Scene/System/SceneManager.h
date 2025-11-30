#pragma once
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Scene/Transitioner/SceneTransitionRequestor.h>
#include <Engine/scene/Base/BaseScene.h>
#include <Game/Scene/Details/SceneType.h>

#include <d3d12.h>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class BaseScene;
class SceneContext;
class DxCore;
class PipelineService;
class PlaySession;

/* ========================================================================
/*		シーン管理クラス
/* ===================================================================== */
class SceneManager
	: public SceneTransitionRequestor {
public:
	explicit SceneManager(DxCore* dx);
	~SceneManager();

	void Initialize();
	void Update(float dt);
	void PostUpdate(ID3D12GraphicsCommandList* cmd, PipelineService* pso);
	void Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso);

	/// <summary>
	/// レンダーターゲットに描画
	/// </summary>
	/// <param name="rt"></param>
	/// <param name="cmd"></param>
	/// <param name="pso"></param>
	void DrawForRenderTarget(class IRenderTarget* rt, ID3D12GraphicsCommandList* cmd, PipelineService* pso);

	/// <summary>
	/// ポストエフェクトの影響を受けない描画
	/// </summary>
	/// <param name="cmd"></param>
	/// <param name="pso"></param>
	void DrawNotAffectedFromPE(ID3D12GraphicsCommandList* cmd, PipelineService* pso);

	/// <summary>
	/// シーン遷移要求
	/// </summary>
	/// <param name="nextScene"></param>
	void RequestSceneChange(SceneType nextScene) override;
	void RequestSceneChange(SceneType nextScene, const SceneTransitionPayload& payload);

	/// <summary>
	/// セッションとバインド
	/// </summary>
	/// <param name="ps"></param>
	void BindPlaySession(PlaySession* ps) { pPlaySession_ = ps; }

	/// <summary>
	/// アクティブなコンテキストを返す
	/// </summary>
	/// <returns></returns>
	SceneContext* ActiveCtx() const;

	/// <summary>
	/// ランタイムをアクティブにするかフラグトグル
	/// </summary>
	/// <returns></returns>
	bool ActiveRuntimeFlag() const;

	/// <summary>
	/// ゲーム終了
	/// </summary>
	/// <returns></returns>
	bool GetIsEndGame() const;

	/// <summary>
	/// コンテキストが変わったらりバインド
	/// </summary>
	void RebindIfContextChanged();

	/// <summary>
	/// シーン追加
	/// </summary>
	/// <param name="type"></param>
	/// <param name="scene"></param>
	/// <returns></returns>
	size_t		  AddScene(SceneType type, std::unique_ptr<BaseScene> scene);

	// getter
	SceneContext* GetCurrentSceneContext() const;
	void		  SetCurrent(size_t index);
	size_t		  GetCurrentIndex() const { return currentIdx_; }

private:
	struct SceneSlot {
		std::unique_ptr<BaseScene>	  scene;
		std::unique_ptr<SceneContext> ctx;
		bool						  assetsLoaded = false;
		int score_;
	};

	std::vector<SceneSlot> slots_;
	size_t				   currentIdx_ = 0;

	std::unordered_map<SceneType, size_t> typeToIndex_;
	std::optional<size_t>				  pendingSwitchIndex_;

	DxCore*		 dx_		   = nullptr;
	PlaySession* pPlaySession_ = nullptr;

	SceneContext* lastBoundCtx_	  = nullptr;
	uint64_t	  lastRuntimeGen_ = 0;
	SceneTransitionPayload pendingPayload_;
};
