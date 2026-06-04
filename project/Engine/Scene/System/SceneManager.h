#pragma once
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Scene/Base/BaseScene.h>
#include <Engine/Scene/Transitioner/IScenePayload.h>
#include <Engine/Scene/Transitioner/SceneTransitionRequestor.h>
#include <Engine/Scene/Utility/SceneUtility.h>


#include <d3d12.h>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class SceneContext;
class PipelineService;

namespace CalyxEngine {
	class PlaySession;
	class PickingPass;
	class GridRenderer;
} // namespace CalyxEngine

class ModelRenderer;

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * SceneManager
	 * - シーン管理クラス
	 * - 複数のシーンの登録、切り替え、更新、描画処理を管理
	 *---------------------------------------------------------------------------------------*/
	class SceneManager {
	public:
		explicit SceneManager(CalyxEngine::DxCore* dx);
		~SceneManager();

		void Initialize();
		void Update(float dt, float alwaysDt);
		void PostUpdate(ID3D12GraphicsCommandList* cmd, PipelineService* pso);
		void Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso);

		void DrawForRenderTarget(class IRenderTarget*		rt,
								 ID3D12GraphicsCommandList* cmd,
								 PipelineService*			pso);

		void DrawNotAffectedFromPE(ID3D12GraphicsCommandList* cmd,
								   PipelineService*			  pso);

		void BindPlaySession(CalyxEngine::PlaySession* ps) { pPlaySession_ = ps; }
		void SetEditorPreviewContext(SceneContext* ctx) { editorPreviewCtx_ = ctx; }

		SceneContext* ActiveCtx() const;
		bool		  ActiveRuntimeFlag() const;
		bool		  GetIsEndGame() const;
		void		  RebindIfContextChanged();

		/// Scene 登録（SceneId で管理）
		size_t AddScene(SceneId id, std::unique_ptr<BaseScene> scene);

		/// 登録済みシーンID一覧取得
		const std::vector<SceneId>& GetRegisteredSceneIds() const { return registeredSceneIds_; }

		/// 指定IDのシーン名取得
		std::string GetSceneName(SceneId id) const;

		void		  SetCurrent(size_t index);
		SceneContext* GetCurrentSceneContext() const;
		size_t		  GetCurrentIndex() const { return currentIdx_; }

		CalyxEngine::ISceneTransitionRequestor& GetTransitionRequestor();

		CalyxEngine::PickingPass* GetPickingPass() const { return pickingPass_.get(); }

	private:
		// ---- internal transition entry ----
		void RequestSceneChangeInternal(SceneId next);
		void RequestSceneChangeInternal(
			SceneId						   next,
			std::unique_ptr<IScenePayload> payload);
		void DrawEditorPreview(IRenderTarget* rt,
							   ID3D12GraphicsCommandList* cmd,
							   PipelineService* pso);
		void DrawSpritesToRenderTarget(IRenderTarget* rt,
									   ID3D12GraphicsCommandList* cmd,
									   PipelineService* pso,
									   bool transitionToShaderResource);

	private:
		struct SceneSlot {
			std::unique_ptr<BaseScene>	  scene;
			std::unique_ptr<SceneContext> ctx;
			bool						  assetsLoaded = false;
		};

		// ---- transition service ----
		class SceneTransitionService final : public ISceneTransitionRequestor {
		public:
			explicit SceneTransitionService(SceneManager& mgr)
				: manager_(mgr) {}

			void RequestSceneChange(SceneId id) override {
				manager_.RequestSceneChangeInternal(id);
			}

			void RequestSceneChange(
				SceneId						   id,
				std::unique_ptr<IScenePayload> payload) override {
				manager_.RequestSceneChangeInternal(id, std::move(payload));
			}

		private:
			SceneManager& manager_;
		};

	private:
		std::vector<SceneSlot>				slots_;
		std::unordered_map<SceneId, size_t> idToIndex_;
		size_t								currentIdx_ = 0;

		std::optional<size_t>		   pendingSwitchIndex_;
		std::unique_ptr<IScenePayload> pendingPayload_;

		CalyxEngine::DxCore*	  dx_			= nullptr;
		CalyxEngine::PlaySession* pPlaySession_ = nullptr;

		SceneContext* lastBoundCtx_	  = nullptr;
		uint64_t	  lastRuntimeGen_ = 0;
		SceneContext* editorPreviewCtx_ = nullptr;

		std::unique_ptr<SceneTransitionService> transitionService_;

		std::vector<SceneId> registeredSceneIds_;

		std::unique_ptr<CalyxEngine::PickingPass> pickingPass_;
		std::unique_ptr<CalyxEngine::GridRenderer> editorGridRenderer_;
		std::unique_ptr<ModelRenderer> editorPreviewModelRenderer_;
	};

} // namespace CalyxEngine
