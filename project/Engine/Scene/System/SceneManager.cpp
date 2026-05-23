#include "SceneManager.h"

// engine
#include <Engine/Application/System/PlaySession.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Graphics/RenderTarget/Collection/RenderTargetCollection.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Scene/Base/IScene.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Graphics/Pipeline/BlendMode/BlendMode.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/Event/BaseEventObject.h>
#include <Engine/Renderer/Grid/GridRenderer.h>
#include <Engine/Renderer/Model/ModelRenderer.h>

// scene
#include "Engine/Scene/Test/TestScene.h"
#include "Game/Scene/Utility/SceneTypeUtil.h"


#include <Engine/Editor/PickingPass.h>

namespace CalyxEngine {
	SceneManager::SceneManager(CalyxEngine::DxCore* dx)
		: dx_(dx) {
		transitionService_ = std::make_unique<SceneTransitionService>(*this);
	}

	SceneManager::~SceneManager() = default;

	CalyxEngine::ISceneTransitionRequestor& SceneManager::GetTransitionRequestor() {
		return *transitionService_;
	}

	//------------------------------------------------------------
	void SceneManager::Initialize() {

		AddScene(GameSceneUtil::ToSceneId(SceneType::TEST),
				 std::make_unique<TestScene>());

		SetCurrent(idToIndex_.at(
			GameSceneUtil::ToSceneId(SceneType::TEST)));

#if defined(_DEBUG) || defined(DEVELOP)
		pickingPass_ = std::make_unique<CalyxEngine::PickingPass>();
		pickingPass_->Initialize(1280, 720);
		editorGridRenderer_ = std::make_unique<CalyxEngine::GridRenderer>();
		editorGridRenderer_->Initialize();
		editorPreviewModelRenderer_ = std::make_unique<ModelRenderer>();
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
			if(pPlaySession_) {
				pPlaySession_->ApplyPendingDebugCameraState(ctx);
			}

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

		if(editorPreviewCtx_) {
			editorPreviewCtx_->MakeCurrent();
			editorPreviewCtx_->PostUpdate(pso, cmd);
		}

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
		auto* debugRT = dx_->GetRenderTargetCollection().Get("DebugView");
		if(editorPreviewCtx_) {
			DrawEditorPreview(debugRT, cmd, pso);
		} else {
			if(auto* ctx = ActiveCtx()) ctx->MakeCurrent();
			CameraManager::SetTypeStatic(CameraType::Debug);
			DrawForRenderTarget(debugRT, cmd, pso);

			if(pickingPass_ && debugRT) {
				auto vp = debugRT->GetViewport();
				pickingPass_->Resize(static_cast<int32_t>(vp.Width), static_cast<int32_t>(vp.Height));
				if(auto* renderer = slots_[currentIdx_].scene->GetModelRenderer()) {
					pickingPass_->Render(cmd, renderer, pso);
				}
				debugRT->SetRenderTarget(cmd);
			}
		}

#endif

		if(!editorPreviewCtx_) {
			if(auto* cam = CameraManager::GetActive()) {
				GraphicsGroup::GetInstance()->SetCommand(cmd, PipelineType::Line, BlendMode::NORMAL);
				cam->SetCommand(cmd, PipelineType::Line);
				PrimitiveDrawer::GetInstance()->Render();
			}
		}
		PrimitiveDrawer::GetInstance()->ClearMesh();
	}

	void SceneManager::DrawEditorPreview(IRenderTarget* rt,
										 ID3D12GraphicsCommandList* cmd,
										 PipelineService* pso) {
		if(!rt || !editorPreviewCtx_) return;

		editorPreviewCtx_->MakeCurrent();
		CameraManager::SetTypeStatic(CameraType::Debug);

		rt->SetRenderTarget(cmd);
		rt->Clear(cmd);

		if(auto* cam = CameraManager::GetActive()) {
			if(editorGridRenderer_) {
				editorGridRenderer_->Render(cmd, pso, cam);
			}
		}

		if(editorPreviewModelRenderer_) {
			editorPreviewModelRenderer_->BeginFrame();

			for(auto* object : editorPreviewCtx_->GetObjectLibrary()->GetAllObjectsRaw()) {
				if(auto* go = dynamic_cast<BaseGameObject*>(object)) {
					switch(go->GetModelType()) {
					case ObjectModelType::ModelType_Static:
						if(auto* model = go->GetStaticModel()) {
							editorPreviewModelRenderer_->RegisterStatic(model, go->GetWorldTransform(), go->GetBillboardMode(), go);
						}
						break;
					case ObjectModelType::ModelType_Animation:
						if(auto* model = go->AnimationModel()) {
							editorPreviewModelRenderer_->RegisterSkinned(model, go->GetWorldTransform(), go);
						}
						break;
					}
				} else if(auto* eventObject = dynamic_cast<BaseEventObject*>(object)) {
					if(auto* model = eventObject->GetModel()) {
						editorPreviewModelRenderer_->RegisterStatic(model, eventObject->GetWorldTransform(), BillboardMode::None, eventObject);
					}
				}
			}

			if(auto* camera = dynamic_cast<Camera3d*>(CameraManager::GetActive())) {
				editorPreviewModelRenderer_->PreCullAndBatch(camera, false);
			} else {
				editorPreviewModelRenderer_->BuildAllVisibleBatches();
			}
			editorPreviewModelRenderer_->DrawAll(cmd,
												 GraphicsGroup::GetInstance()->GetDevice().Get(),
												 rt,
												 pso,
												 editorPreviewCtx_->GetLightLibrary(),
												 nullptr);
		}

		editorPreviewCtx_->GetFxSystem()->Render(pso, cmd);
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

} // namespace CalyxEngine
