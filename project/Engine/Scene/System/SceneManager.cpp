#include "SceneManager.h"

// engine
#include <Engine/Application/Settings/EngineSettings.h>
#include <Engine/Application/System/PlaySession.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Editor/AssetPreviewManager.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Foundation/Log/EngineLogger.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Graphics/Pipeline/BlendMode/BlendMode.h>
#include <Engine/Graphics/RenderTarget/Collection/RenderTargetCollection.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/Event/BaseEventObject.h>
#include <Engine/Renderer/Grid/GridRenderer.h>
#include <Engine/Renderer/Model/ModelRenderer.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Scene/Base/IScene.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/System/Command/Manager/CommandManager.h>

#include <Engine/Editor/PickingPass.h>

#include <algorithm>
#include <cmath>

namespace {
	constexpr uint32_t kMaxViewportRenderWidth = 1920;
	constexpr uint32_t kMaxViewportRenderHeight = 1080;

	uint32_t ToRenderExtent(float value) {
		return static_cast<uint32_t>((std::max)(1.0f, std::ceil(value)));
	}

	void ResizeTargetToViewport(IRenderTarget* rt, const CalyxEngine::Vector2& size) {
		if(!rt || size.x <= 0.0f || size.y <= 0.0f) return;

		uint32_t width = ToRenderExtent(size.x);
		uint32_t height = ToRenderExtent(size.y);
		const float scale = (std::min)(
			static_cast<float>(kMaxViewportRenderWidth) / static_cast<float>(width),
			static_cast<float>(kMaxViewportRenderHeight) / static_cast<float>(height));

		if(scale < 1.0f) {
			width = (std::max)(1u, static_cast<uint32_t>(std::floor(static_cast<float>(width) * scale)));
			height = (std::max)(1u, static_cast<uint32_t>(std::floor(static_cast<float>(height) * scale)));
		}

		rt->Resize(width, height);
	}

	CalyxEngine::Vector3 SafeNormalize(
		const CalyxEngine::Vector3& value,
		const CalyxEngine::Vector3& fallback) {
		if(value.LengthSquared() <= 1.0e-8f) {
			return fallback;
		}
		return value.Normalize();
	}

	void DrawCameraViewAxis(BaseCamera* camera) {
		if(!camera) return;

		const WorldTransform& transform = camera->GetWorldTransform();
		const CalyxEngine::Matrix4x4& world = transform.matrix.world;
		const CalyxEngine::Vector3 cameraPos = transform.GetWorldPosition();
		const CalyxEngine::Vector3 right = SafeNormalize(
			{world.m[0][0], world.m[0][1], world.m[0][2]},
			CalyxEngine::Vector3::Right());
		const CalyxEngine::Vector3 up = SafeNormalize(
			{world.m[1][0], world.m[1][1], world.m[1][2]},
			CalyxEngine::Vector3::Up());
		const CalyxEngine::Vector3 forward = SafeNormalize(
			{world.m[2][0], world.m[2][1], world.m[2][2]},
			CalyxEngine::Vector3::Forward());

		const float distance = 2.0f;
		const float halfHeight = std::tan(camera->GetFovY() * 0.5f) * distance;
		const float halfWidth = halfHeight * camera->GetAspectRatio();
		const float axisLength = halfHeight * 0.18f;
		const CalyxEngine::Vector3 origin =
			cameraPos +
			forward * distance -
			right * (halfWidth * 0.78f) -
			up * (halfHeight * 0.70f);

		auto* drawer = PrimitiveDrawer::GetInstance();
		drawer->DrawViewportLine3d(origin, origin + CalyxEngine::Vector3::Right() * axisLength, {1.0f, 0.15f, 0.12f, 1.0f});
		drawer->DrawViewportLine3d(origin, origin + CalyxEngine::Vector3::Up() * axisLength, {0.15f, 0.35f, 1.0f, 1.0f});
		drawer->DrawViewportLine3d(origin, origin + CalyxEngine::Vector3::Forward() * axisLength, {0.20f, 0.90f, 0.30f, 1.0f});
	}
}

namespace CalyxEngine {
	SceneManager::SceneManager(DxCore* dx)
		: dx_(dx) {
		transitionService_ = std::make_unique<SceneTransitionService>(*this);
	}

	SceneManager::~SceneManager() = default;

	ISceneTransitionRequestor& SceneManager::GetTransitionRequestor() {
		return *transitionService_;
	}

	//------------------------------------------------------------
	void SceneManager::Initialize() {
		EngineLogger::GetInstance().Add(LogLevel::Info, LogCategory::Engine, "Scene manager initialization started.", "SceneManager");
#if defined(_DEBUG) || defined(DEVELOP)
		pickingPass_ = std::make_unique<PickingPass>();
		pickingPass_->Initialize(1280, 720);
		editorGridRenderer_ = std::make_unique<GridRenderer>();
		editorGridRenderer_->Initialize();
		editorPreviewModelRenderer_ = std::make_unique<ModelRenderer>();
#endif
		EngineLogger::GetInstance().Add(LogLevel::Info, LogCategory::Engine, "Scene manager initialization completed.", "SceneManager");
	}

	bool SceneManager::OpenScene(const std::filesystem::path& scenePath) {
		if(scenePath.empty()) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Editor, "Scene open failed because the path is empty.", "SceneManager");
			return false;
		}
		const bool rebuildRuntime = pPlaySession_ && pPlaySession_->IsRuntime();

		SceneContext* previousContext = SceneContext::Current();
		auto nextContext = std::make_unique<SceneContext>();
		nextContext->Initialize(false);
		if(!SceneSerializer::Load(*nextContext, scenePath.generic_string())) {
			if(previousContext) previousContext->MakeCurrent();
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Editor, "Scene deserialization failed: " + scenePath.generic_string(), "SceneManager");
			return false;
		}
		nextContext->SetScenePath(scenePath.generic_string());
		nextContext->SetSceneTransitionRequestor(&GetTransitionRequestor());

		if(activeScene_.scene) activeScene_.scene->OnExit();

		// Files opened by the editor use the common scene runtime. Scene-specific
		// behaviour belongs to serialized SceneObjects, not a BaseScene subclass.
		activeScene_.scene = std::make_unique<BaseScene>();
		activeScene_.scene->SetSceneName(nextContext->GetSceneName());
		activeScene_.scene->SetTransitionRequestor(&GetTransitionRequestor());
		activeScene_.ctx = std::move(nextContext);
		activeScene_.assetsLoaded = false;

		CommandManager::GetInstance()->ClearHistory();
		if(pPlaySession_) {
			pPlaySession_->BindEditorContext(activeScene_.ctx.get());
			if(rebuildRuntime) pPlaySession_->RebuildRuntimeFromEditor(activeScene_.ctx.get());
		}
		lastBoundCtx_ = nullptr;
		lastRuntimeGen_ = 0;
		RebindIfContextChanged();
		EngineLogger::GetInstance().Add(LogLevel::Info, LogCategory::Editor, "Scene opened successfully: " + scenePath.generic_string(), "SceneManager");
		return true;
	}

	//------------------------------------------------------------
	bool SceneManager::OpenScene(const Guid& sceneAssetGuid) {
		const AssetRecord* record = AssetDatabase::GetInstance()->Get(sceneAssetGuid);
		if(!record || record->type != AssetType::Scene) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Asset, "Scene asset GUID could not be resolved: " + sceneAssetGuid.ToString(), "SceneManager");
			return false;
		}
		return OpenScene(record->sourcePath);
	}

	void SceneManager::RequestSceneChange(const std::filesystem::path& scenePath) { RequestSceneChangeInternal(scenePath); }
	void SceneManager::RequestSceneChange(const Guid& sceneAssetGuid) { RequestSceneChangeInternal(sceneAssetGuid); }

	//------------------------------------------------------------


		// 新しい Editor ctx を PlaySession に通知


		// 再生中なら新しい Editor 内容から Runtime を再構築


	//------------------------------------------------------------
	SceneContext* SceneManager::GetCurrentSceneContext() const {
		return activeScene_.ctx.get();
	}

	std::filesystem::path SceneManager::GetCurrentScenePath() const {
		auto* context = GetCurrentSceneContext();
		return context ? std::filesystem::path(context->GetScenePath()) : std::filesystem::path{};
	}

	//------------------------------------------------------------
	SceneContext* SceneManager::ActiveCtx() const {
		if(pPlaySession_ && pPlaySession_->IsRuntime()) return pPlaySession_->GetContext();
		if(pPlaySession_) return pPlaySession_->GetContext();
		return activeScene_.ctx.get();
	}

	bool SceneManager::ActiveRuntimeFlag() const {
		if(pPlaySession_) return pPlaySession_->IsRuntime();
		return activeScene_.ctx ? activeScene_.ctx->IsRuntime() : false;
	}

	void SceneManager::SetEditorPreviewContext(SceneContext* ctx) {
		editorPreviewCtx_ = ctx;
	}

	void SceneManager::ClearAllContexts() {
		EngineLogger::GetInstance().Add(LogLevel::Info, LogCategory::Engine, "Clearing all scene contexts.", "SceneManager");
		editorPreviewCtx_ = nullptr;
		lastBoundCtx_	 = nullptr;
		lastRuntimeGen_	 = 0;

		CommandManager::GetInstance()->ClearHistory();

		if(pPlaySession_) {
			pPlaySession_->ClearRuntimeContext();
		}

		if(activeScene_.scene) activeScene_.scene->OnExit();
		if(activeScene_.ctx) activeScene_.ctx->Clear();
		activeScene_ = {};
	}

	bool SceneManager::GetIsEndGame() const { return activeScene_.scene && activeScene_.scene->GetIsEndGame(); }

	void SceneManager::RebindIfContextChanged() {
		SceneContext* ctx = ActiveCtx();
		if(!ctx) return;

		const uint64_t gen = pPlaySession_ ? pPlaySession_->RuntimeGeneration() : 0;

		if(ctx != lastBoundCtx_ || gen != lastRuntimeGen_) {
			auto& slot = activeScene_;

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
		if(!activeScene_.scene) return;

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

		auto& slot = activeScene_;
		slot.scene->InjectContext(ctx);
		slot.scene->Update(dt);

		if(pendingScenePath_.has_value()) {
			auto nextPath = std::move(*pendingScenePath_);
			pendingScenePath_.reset();
			OpenScene(nextPath);
		}
	}

	//------------------------------------------------------------
	void SceneManager::PostUpdate(ID3D12GraphicsCommandList* cmd, PipelineService* pso) {
		if(!activeScene_.scene) return;

		if(editorPreviewCtx_) {
			editorPreviewCtx_->MakeCurrent();
			editorPreviewCtx_->PostUpdate(pso, cmd);
		}

		RebindIfContextChanged();
		if(auto* ctx = ActiveCtx()) {
			ctx->MakeCurrent();
		}
		activeScene_.scene->PostUpdate(cmd, pso);
	}

	//------------------------------------------------------------
	void SceneManager::Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso) {
		if(!activeScene_.scene) return;
		RebindIfContextChanged();

		if(auto* ctx = ActiveCtx()) ctx->MakeCurrent();

		CameraManager::SetTypeStatic(CameraType::Default);
		auto* offscreen = dx_->GetRenderTargetCollection().Get("Offscreen");
		ResizeTargetToViewport(offscreen, CameraManager::GetViewportSizeStatic(ViewportType::VIEWPORT_MAIN));
		DrawForRenderTarget(offscreen, cmd, pso);
		const bool renderDebugLines = EngineSettings::GetInstance()->GetData().editor.renderDebugLinesInViewports;
		if(renderDebugLines) {
			CameraManager::SetTypeStatic(CameraType::Default);
			if(EngineSettings::GetInstance()->GetData().editor.showEditorGrid && offscreen) {
				offscreen->SetRenderTarget(cmd);
				if(auto* cam = CameraManager::GetActive()) {
					if(editorGridRenderer_) {
						editorGridRenderer_->Render(cmd, pso, cam);
					}
				}
			}
			RenderDebugPrimitivesToRenderTarget(offscreen, cmd, false);
		}
		RenderViewportAxisToRenderTarget(offscreen, cmd);

#if defined(_DEBUG) || defined(DEVELOP)
		auto* debugRT = dx_->GetRenderTargetCollection().Get("DebugView");
		if(renderDebugView_) {
			ResizeTargetToViewport(debugRT, CameraManager::GetViewportSizeStatic(ViewportType::VIEWPORT_DEBUG));
			if(editorPreviewCtx_) {
				DrawEditorPreview(debugRT, cmd, pso);
			} else {
				if(auto* ctx = ActiveCtx()) ctx->MakeCurrent();
				CameraManager::SetTypeStatic(CameraType::Debug);
				DrawForRenderTarget(debugRT, cmd, pso);

				if(renderPicking_ && pickingPass_ && debugRT) {
					auto vp = debugRT->GetViewport();
					pickingPass_->Resize(static_cast<int32_t>(vp.Width), static_cast<int32_t>(vp.Height));
					if(auto* renderer = activeScene_.scene->GetModelRenderer()) {
						pickingPass_->Render(cmd, renderer, pso);
					}
					debugRT->SetRenderTarget(cmd);
				}
				if(EngineSettings::GetInstance()->GetData().editor.showEditorGrid && debugRT) {
					debugRT->SetRenderTarget(cmd);
					if(auto* cam = CameraManager::GetActive()) {
						if(editorGridRenderer_) {
							editorGridRenderer_->Render(cmd, pso, cam);
						}
					}
				}
				if(renderDebugLines) {
					CameraManager::SetTypeStatic(CameraType::Debug);
					RenderDebugPrimitivesToRenderTarget(debugRT, cmd, true);
				}
				CameraManager::SetTypeStatic(CameraType::Debug);
				RenderViewportAxisToRenderTarget(debugRT, cmd);
			}
		}

#endif

		if(auto* previewRT = dx_->GetRenderTargetCollection().Get("AssetPreview")) {
			if(auto* previews = AssetPreviewManager::GetInstance()) {
				SceneContext* previousContext = SceneContext::Current();
				previews->ProcessRenderQueue(cmd, pso, previewRT, 1);
				if(previousContext) previousContext->MakeCurrent();
			}
		}

		PrimitiveDrawer::GetInstance()->ClearMesh();
	}

	void SceneManager::SetEditorViewportRenderState(bool renderDebugView, bool renderPicking) {
		renderDebugView_ = renderDebugView;
		renderPicking_	 = renderDebugView && renderPicking;
	}

	void SceneManager::DrawEditorPreview(IRenderTarget*				rt,
										 ID3D12GraphicsCommandList* cmd,
										 PipelineService*			pso) {
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
							editorPreviewModelRenderer_->RegisterStatic(model, go->GetRenderWorldTransform(), go->GetBillboardMode(), go);
						}
						break;
					case ObjectModelType::ModelType_Animation:
						if(auto* model = go->AnimationModel()) {
							editorPreviewModelRenderer_->RegisterSkinned(model, go->GetRenderWorldTransform(), go);
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

	void SceneManager::RenderDebugPrimitivesToRenderTarget(IRenderTarget* rt,
														   ID3D12GraphicsCommandList* cmd,
														   bool includeDebugViewOnly) {
		if(!rt || !cmd || editorPreviewCtx_) return;

		rt->SetRenderTarget(cmd);
		if(auto* cam = CameraManager::GetActive()) {
			GraphicsGroup::GetInstance()->SetCommand(cmd, PipelineType::Line, BlendMode::NORMAL);
			cam->SetCommand(cmd, PipelineType::Line);
			PrimitiveDrawer::GetInstance()->Render(includeDebugViewOnly, LineDepthMode::DepthTest);

			GraphicsGroup::GetInstance()->SetCommand(cmd, PipelineType::LineNoDepth, BlendMode::NORMAL);
			cam->SetCommand(cmd, PipelineType::LineNoDepth);
			PrimitiveDrawer::GetInstance()->Render(includeDebugViewOnly, LineDepthMode::NoDepthTest);
		}
	}

	void SceneManager::RenderViewportAxisToRenderTarget(IRenderTarget* rt,
														ID3D12GraphicsCommandList* cmd) {
		if(!rt || !cmd || editorPreviewCtx_) return;

		rt->SetRenderTarget(cmd);
		if(auto* cam = CameraManager::GetActive()) {
			DrawCameraViewAxis(cam);
			GraphicsGroup::GetInstance()->SetCommand(cmd, PipelineType::LineNoDepth, BlendMode::NORMAL);
			cam->SetCommand(cmd, PipelineType::LineNoDepth);
			PrimitiveDrawer::GetInstance()->RenderViewportLines();
			PrimitiveDrawer::GetInstance()->ClearViewportLines();
		}
	}

	void SceneManager::DrawSpritesToRenderTarget(IRenderTarget* rt, ID3D12GraphicsCommandList* cmd, PipelineService* pso, bool transitionToShaderResource) {
		if(!rt) return;

		rt->TransitionTo(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
		rt->SetRenderTarget(cmd);

		activeScene_.scene->DrawSpritesOnly(cmd, pso);

		if(transitionToShaderResource) {
			rt->TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
	}

	//------------------------------------------------------------
	void SceneManager::DrawForRenderTarget(IRenderTarget*			  rt,
										   ID3D12GraphicsCommandList* cmd,
										   PipelineService*			  pso) {

		if(!rt) return;
		rt->SetRenderTarget(cmd);
		rt->Clear(cmd);

		auto& slot = activeScene_;
		slot.scene->Draw(cmd, pso, rt);
	}

	//------------------------------------------------------------
	void SceneManager::DrawNotAffectedFromPE(ID3D12GraphicsCommandList* cmd, PipelineService* pso) {
		if(!activeScene_.scene) return;
		auto* postOutput = dx_->GetRenderTargetCollection().Get("PostEffectOutput");
		DrawSpritesToRenderTarget(postOutput, cmd, pso, true);

		auto* backBuffer = dx_->GetRenderTargetCollection().Get("BackBuffer");
		DrawSpritesToRenderTarget(backBuffer, cmd, pso, false);
	}

	void SceneManager::RequestSceneChangeInternal(const std::filesystem::path& scenePath, std::unique_ptr<IScenePayload> payload) {
		if(scenePath.empty()) {
			EngineLogger::GetInstance().Add(LogLevel::Warning, LogCategory::Game, "Scene change request ignored because the scene path is empty.", "SceneManager");
			return;
		}
		pendingPayload_ = std::move(payload);
		pendingScenePath_ = scenePath;
		EngineLogger::GetInstance().Add(LogLevel::Trace, LogCategory::Game, "Scene change queued: " + scenePath.generic_string(), "SceneManager");
	}

	void SceneManager::RequestSceneChangeInternal(const Guid& sceneAssetGuid, std::unique_ptr<IScenePayload> payload) {
		const AssetRecord* record = AssetDatabase::GetInstance()->Get(sceneAssetGuid);
		if(!record || record->type != AssetType::Scene) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Asset, "Scene transition GUID could not be resolved: " + sceneAssetGuid.ToString(), "SceneManager");
			return;
		}
		RequestSceneChangeInternal(record->sourcePath, std::move(payload));
	}

} // namespace CalyxEngine
