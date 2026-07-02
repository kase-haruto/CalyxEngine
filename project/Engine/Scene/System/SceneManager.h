#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Scene/Base/BaseScene.h>
#include <Engine/Scene/Transitioner/IScenePayload.h>
#include <Engine/Scene/Transitioner/SceneTransitionRequestor.h>

#include <d3d12.h>
#include <filesystem>
#include <memory>
#include <optional>

class SceneContext;
class PipelineService;
class ModelRenderer;

namespace CalyxEngine {
	class PlaySession;
	class PickingPass;
	class GridRenderer;

	class CALYX_API SceneManager {
	public:
		explicit SceneManager(DxCore* dx);
		~SceneManager();

		void Initialize();
		bool OpenScene(const std::filesystem::path& scenePath);
		bool OpenScene(const Guid& sceneAssetGuid);
		void RequestSceneChange(const std::filesystem::path& scenePath);
		void RequestSceneChange(const Guid& sceneAssetGuid);

		void Update(float dt, float alwaysDt);
		void PostUpdate(ID3D12GraphicsCommandList* cmd, PipelineService* pso);
		void Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso);
		void DrawForRenderTarget(class IRenderTarget* rt, ID3D12GraphicsCommandList* cmd, PipelineService* pso);
		void DrawNotAffectedFromPE(ID3D12GraphicsCommandList* cmd, PipelineService* pso);

		void BindPlaySession(PlaySession* ps) { pPlaySession_ = ps; }
		SceneContext* ActiveCtx() const;
		bool ActiveRuntimeFlag() const;
		bool GetIsEndGame() const;
		void RebindIfContextChanged();
		bool HasScene() const { return activeScene_.scene != nullptr; }

		void SetEditorPreviewContext(SceneContext* ctx);
		void SetEditorViewportRenderState(bool renderDebugView, bool renderPicking);
		void ClearAllContexts();
		SceneContext* GetCurrentSceneContext() const;
		std::filesystem::path GetCurrentScenePath() const;
		ISceneTransitionRequestor& GetTransitionRequestor();
		PickingPass* GetPickingPass() const { return pickingPass_.get(); }

	private:
		void RequestSceneChangeInternal(const std::filesystem::path& scenePath, std::unique_ptr<IScenePayload> payload = nullptr);
		void RequestSceneChangeInternal(const Guid& sceneAssetGuid, std::unique_ptr<IScenePayload> payload = nullptr);
		void DrawEditorPreview(IRenderTarget* rt, ID3D12GraphicsCommandList* cmd, PipelineService* pso);
		void RenderDebugPrimitivesToRenderTarget(IRenderTarget* rt, ID3D12GraphicsCommandList* cmd, bool includeDebugViewOnly);
		void RenderViewportAxisToRenderTarget(IRenderTarget* rt, ID3D12GraphicsCommandList* cmd);
		void DrawSpritesToRenderTarget(IRenderTarget* rt, ID3D12GraphicsCommandList* cmd, PipelineService* pso, bool transitionToShaderResource);

		struct RuntimeScene {
			std::unique_ptr<BaseScene> scene;
			std::unique_ptr<SceneContext> ctx;
			bool assetsLoaded = false;
		};

		class SceneTransitionService final : public ISceneTransitionRequestor {
		public:
			explicit SceneTransitionService(SceneManager& manager) : manager_(manager) {}
			void RequestSceneChange(const std::filesystem::path& path) override { manager_.RequestSceneChangeInternal(path); }
			void RequestSceneChange(const std::filesystem::path& path, std::unique_ptr<IScenePayload> payload) override { manager_.RequestSceneChangeInternal(path, std::move(payload)); }
			void RequestSceneChange(const Guid& guid) override { manager_.RequestSceneChangeInternal(guid); }
			void RequestSceneChange(const Guid& guid, std::unique_ptr<IScenePayload> payload) override { manager_.RequestSceneChangeInternal(guid, std::move(payload)); }
		private:
			SceneManager& manager_;
		};

		RuntimeScene activeScene_;
		std::optional<std::filesystem::path> pendingScenePath_;
		std::unique_ptr<IScenePayload> pendingPayload_;
		DxCore* dx_ = nullptr;
		PlaySession* pPlaySession_ = nullptr;
		SceneContext* lastBoundCtx_ = nullptr;
		uint64_t lastRuntimeGen_ = 0;
		SceneContext* editorPreviewCtx_ = nullptr;
		bool renderDebugView_ = true;
		bool renderPicking_ = true;
		std::unique_ptr<SceneTransitionService> transitionService_;
		std::unique_ptr<PickingPass> pickingPass_;
		std::unique_ptr<GridRenderer> editorGridRenderer_;
		std::unique_ptr<ModelRenderer> editorPreviewModelRenderer_;
	};
}
