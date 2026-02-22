#include "SceneManager.h"

// engine
#include <Engine/Application/System/PlaySession.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Graphics/RenderTarget/Collection/RenderTargetCollection.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Scene/Base/IScene.h>
#include <Engine/Scene/Context/SceneContext.h>

// scene
#include "Engine/Scene/Test/TestScene.h"
#include "Game/Scene/Clear/ClearScene.h"
#include "Game/Scene/Defeat/DefeatScene.h"
#include "Game/Scene/Utility/SceneTypeUtil.h"

#include <Engine/Scene/Title/TitleScene.h>
#include <Game/Scene/Game/GameScene.h>

#include <Engine/Editor/PickingPass.h>

namespace CalyxScene {
	SceneManager::SceneManager(CalyxGraphics::DxCore* dx)
		: dx_(dx) {
		transitionService_ = std::make_unique<SceneTransitionService>(*this);
	}

	SceneManager::~SceneManager() = default;

	CalyxScene::ISceneTransitionRequestor& SceneManager::GetTransitionRequestor() {
		return *transitionService_;
	}

	//------------------------------------------------------------
	void SceneManager::Initialize() {
		AddScene(GameSceneUtil::ToSceneId(SceneType::TITLE),
				 std::make_unique<TitleScene>());

		AddScene(GameSceneUtil::ToSceneId(SceneType::PLAY),
				 std::make_unique<GameScene>());

		AddScene(GameSceneUtil::ToSceneId(SceneType::TEST),
				 std::make_unique<TestScene>());

		AddScene(GameSceneUtil::ToSceneId(SceneType::DEFEAT),
				 std::make_unique<DefeatScene>());

		AddScene(GameSceneUtil::ToSceneId(SceneType::CLEAR),
				 std::make_unique<ClearScene>());

		SetCurrent(idToIndex_.at(
			GameSceneUtil::ToSceneId(SceneType::TITLE)));

#if defined(_DEBUG) || defined(DEVELOP)
		pickingPass_ = std::make_unique<CalyxEditor::PickingPass>();
		pickingPass_->Initialize(1280, 720);
#endif
	}

	//------------------------------------------------------------
	size_t SceneManager::AddScene(SceneId id, std::unique_ptr<BaseScene> scene) {
		SceneSlot slot;
		slot.scene = std::move(scene);
		slot.ctx   = std::make_unique<SceneContext>();
		slot.ctx->Initialize(false);

		slot.scene->SetTransitionRequestor(&GetTransitionRequestor());

		slots_.push_back(std::move(slot));
		size_t index   = slots_.size() - 1;
		idToIndex_[id] = index;
		registeredSceneIds_.push_back(id);
		return index;
	}

	//------------------------------------------------------------
	void SceneManager::SetCurrent(size_t index) {
		if(index >= slots_.size()) return;

		if(pPlaySession_ && pPlaySession_->ExitRequested()) {
			pPlaySession_->FinalizeExitCleanup();
		}

		if(!slots_.empty()) {
			slots_[currentIdx_].scene->OnExit();
		}

		currentIdx_ = index;
		auto& s		= slots_[currentIdx_];

		// 新しい Editor ctx を PlaySession に通知
		if(pPlaySession_) pPlaySession_->BindEditorContext(s.ctx.get());

		// 再生中なら新しい Editor 内容から Runtime を再構築
		if(pPlaySession_ && pPlaySession_->IsRuntime()) {
			pPlaySession_->RebuildRuntimeFromEditor(s.ctx.get());
		}

		RebindIfContextChanged();
	}

	//------------------------------------------------------------
	SceneContext* SceneManager::GetCurrentSceneContext() const {
		if(slots_.empty()) return nullptr;
		return slots_[currentIdx_].ctx.get();
	}

	//------------------------------------------------------------
	SceneContext* SceneManager::ActiveCtx() const {
		if(pPlaySession_) return pPlaySession_->GetContext();
		if(slots_.empty()) return nullptr;
		return slots_[currentIdx_].ctx.get();
	}

	bool SceneManager::ActiveRuntimeFlag() const {
		if(pPlaySession_) return pPlaySession_->IsRuntime();
		if(slots_.empty()) return false;
		return slots_[currentIdx_].ctx->IsRuntime();
	}

	bool SceneManager::GetIsEndGame() const { return slots_[currentIdx_].scene->GetIsEndGame(); }

	void SceneManager::RebindIfContextChanged() {
		SceneContext* ctx = ActiveCtx();
		if(!ctx) return;

		const uint64_t gen = pPlaySession_ ? pPlaySession_->RuntimeGeneration() : 0;

		if(ctx != lastBoundCtx_ || gen != lastRuntimeGen_) {
			auto& slot = slots_[currentIdx_];

			// 前回のctxにぶら下がるキャッシュを捨てる
			slot.scene->OnExit();

			ctx->MakeCurrent();
			slot.scene->InjectContext(ctx);

			if(!slot.assetsLoaded) {
				slot.scene->LoadAssets();
				slot.assetsLoaded = true;
			}

			// payload があれば、次のシーンに渡す
			if(pendingPayload_) {
				slot.scene->OnPayload(std::move(pendingPayload_));
			}

			// 毎回初期化
			slot.scene->Initialize();
			slot.scene->OnEnter();

			lastBoundCtx_	= ctx;
			lastRuntimeGen_ = gen;
		}
	}

	void SceneManager::Update(float dt, float alwaysDt) {
		if(slots_.empty()) return;

		if(pPlaySession_ && pPlaySession_->ExitRequested()) {
			pPlaySession_->FinalizeExitCleanup();
			lastBoundCtx_	= nullptr;
			lastRuntimeGen_ = 0;
		}

		RebindIfContextChanged();

		SceneContext* ctx = ActiveCtx();
		if(!ctx) return;

		ctx->MakeCurrent();
		ctx->Update(dt, alwaysDt, ActiveRuntimeFlag());

		auto& slot = slots_[currentIdx_];
		slot.scene->InjectContext(ctx);
		slot.scene->Update(dt);

		if(pendingSwitchIndex_.has_value()) {
			SetCurrent(*pendingSwitchIndex_);
			pendingSwitchIndex_.reset();
		}
	}

	//------------------------------------------------------------
	void SceneManager::PostUpdate(ID3D12GraphicsCommandList* cmd, PipelineService* pso) {
		if(slots_.empty()) return;

		RebindIfContextChanged();
		if(auto* ctx = ActiveCtx()) {
			ctx->MakeCurrent();
		}
		slots_[currentIdx_].scene->PostUpdate(cmd, pso);
	}

	//------------------------------------------------------------
	void SceneManager::Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso) {
		if(slots_.empty()) return;
		RebindIfContextChanged();

		if(auto* ctx = ActiveCtx()) ctx->MakeCurrent();

		CameraManager::SetTypeStatic(CameraType::Default);
		auto* offscreen = dx_->GetRenderTargetCollection().Get("Offscreen");
		DrawForRenderTarget(offscreen, cmd, pso);

#if defined(_DEBUG) || defined(DEVELOP)
		if(auto* ctx = ActiveCtx()) ctx->MakeCurrent();
		CameraManager::SetTypeStatic(CameraType::Debug);
		auto* debugRT = dx_->GetRenderTargetCollection().Get("DebugView");

		if(pickingPass_ && debugRT) {
			auto vp = debugRT->GetViewport();
			pickingPass_->Resize(static_cast<int32_t>(vp.Width), static_cast<int32_t>(vp.Height));
			if(auto* renderer = slots_[currentIdx_].scene->GetModelRenderer()) {
				// ピッキングの前にデバッグカメラ視点でカリング結果を更新する
				if(auto* debugCam = CameraManager::GetDebug()) {
					renderer->PreCullAndBatch(debugCam);
				}
				pickingPass_->Render(cmd, renderer, pso);
			}
		}

		DrawForRenderTarget(debugRT, cmd, pso);
#endif

		if(auto* cam = CameraManager::GetActive()) {
			GraphicsGroup::GetInstance()->SetCommand(cmd, PipelineType::Line, BlendMode::NORMAL);
			cam->SetCommand(cmd, PipelineType::Line);
			PrimitiveDrawer::GetInstance()->Render();
		}
		PrimitiveDrawer::GetInstance()->ClearMesh();
	}

	//------------------------------------------------------------
	void SceneManager::DrawForRenderTarget(IRenderTarget*			  rt,
										   ID3D12GraphicsCommandList* cmd,
										   PipelineService*			  pso) {

		if(!rt) return;
		rt->SetRenderTarget(cmd);
		rt->Clear(cmd);

		auto& slot = slots_[currentIdx_];
		slot.scene->Draw(cmd, pso, rt);

		// gameViewパネルにもスプライトを描画する
		if(rt->GetRenderTargetType() != RenderTargetType::DebugView) {
			slot.scene->DrawSpritesOnly(cmd, pso);
		}
	}

	//------------------------------------------------------------
	void SceneManager::DrawNotAffectedFromPE(ID3D12GraphicsCommandList* cmd, PipelineService* pso) {
		if(slots_.empty()) return;
		slots_[currentIdx_].scene->DrawSpritesOnly(cmd, pso);
	}

	void SceneManager::RequestSceneChangeInternal(SceneId next) {
		auto it = idToIndex_.find(next);
		if(it == idToIndex_.end()) return;
		pendingSwitchIndex_ = it->second;
	}

	void SceneManager::RequestSceneChangeInternal(
		SceneId						   next,
		std::unique_ptr<IScenePayload> payload) {

		pendingPayload_ = std::move(payload);
		RequestSceneChangeInternal(next);
	}

	std::string SceneManager::GetSceneName(SceneId id) const {
		auto it = idToIndex_.find(id);
		if(it == idToIndex_.end()) return "Unknown Scene";

		size_t index = it->second;
		if(index >= slots_.size()) return "Invalid Index";

		return slots_[index].scene->GetSceneName();
	}

} // namespace CalyxScene