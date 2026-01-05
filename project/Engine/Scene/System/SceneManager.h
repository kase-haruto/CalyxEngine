#pragma once
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Scene/Transitioner/IScenePayload.h>
#include <Engine/Scene/Transitioner/SceneTransitionRequestor.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Scene/Base/BaseScene.h>

#include <d3d12.h>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class SceneContext;
class PipelineService;

namespace CalyxEditor {
	class PlaySession;
}

namespace CalyxScene {

	class SceneManager {
	public:
		explicit SceneManager(CalyxGraphics::DxCore* dx);
		~SceneManager();

		void Initialize();
		void Update(float dt);
		void PostUpdate(ID3D12GraphicsCommandList* cmd, PipelineService* pso);
		void Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso);

		void DrawForRenderTarget(class IRenderTarget* rt,
								 ID3D12GraphicsCommandList* cmd,
								 PipelineService* pso);

		void DrawNotAffectedFromPE(ID3D12GraphicsCommandList* cmd,
								   PipelineService* pso);

		void BindPlaySession(CalyxEditor::PlaySession* ps) { pPlaySession_ = ps; }

		SceneContext* ActiveCtx() const;
		bool		  ActiveRuntimeFlag() const;
		bool		  GetIsEndGame() const;
		void		  RebindIfContextChanged();

		/// Scene 登録（SceneId で管理）
		size_t AddScene(SceneId id, std::unique_ptr<BaseScene> scene);

		void		  SetCurrent(size_t index);
		SceneContext* GetCurrentSceneContext() const;
		size_t		  GetCurrentIndex() const { return currentIdx_; }

		CalyxScene::ISceneTransitionRequestor& GetTransitionRequestor();

	private:
		// ---- internal transition entry ----
		void RequestSceneChangeInternal(SceneId next);
		void RequestSceneChangeInternal(
			SceneId next,
			std::unique_ptr<IScenePayload> payload);

	private:
		struct SceneSlot {
			std::unique_ptr<BaseScene> scene;
			std::unique_ptr<SceneContext> ctx;
			bool assetsLoaded = false;
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
				SceneId id,
				std::unique_ptr<IScenePayload> payload) override {
				manager_.RequestSceneChangeInternal(id, std::move(payload));
			}

		private:
			SceneManager& manager_;
		};

	private:
		std::vector<SceneSlot> slots_;
		std::unordered_map<SceneId, size_t> idToIndex_;
		size_t currentIdx_ = 0;

		std::optional<size_t> pendingSwitchIndex_;
		std::unique_ptr<IScenePayload> pendingPayload_;

		CalyxGraphics::DxCore* dx_ = nullptr;
		CalyxEditor::PlaySession* pPlaySession_ = nullptr;

		SceneContext* lastBoundCtx_ = nullptr;
		uint64_t lastRuntimeGen_ = 0;

		std::unique_ptr<SceneTransitionService> transitionService_;
	};

} // namespace CalyxScene
