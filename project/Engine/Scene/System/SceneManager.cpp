#include "SceneManager.h"

// engine
#include <Engine/Application/Settings/EngineSettings.h>
#include <Engine/Application/System/PlaySession.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Editor/AssetPreviewManager.h>
#include <Engine/Foundation/Log/EngineLogger.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>
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
#include <Engine/Scene/Fade/FadeBlackOutEffect.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/System/Command/Manager/CommandManager.h>

#include <Engine/Editor/PickingPass.h>

#include <algorithm>
#include <cmath>

namespace {
	// エディタビューポートの描画負荷を抑えるため、内部レンダーターゲットの最大解像度を制限する
	constexpr uint32_t kMaxViewportRenderWidth	= 1920;
	constexpr uint32_t kMaxViewportRenderHeight = 1080;

	/////////////////////////////////////////////////////////////////////////////////////////
	//    ビューポートサイズを描画可能な整数の大きさへ変換する
	/////////////////////////////////////////////////////////////////////////////////////////
	uint32_t ToRenderExtent(float value) {
		// 小数点以下を切り上げ、描画領域が0にならないよう1以上へ補正する
		return static_cast<uint32_t>((std::max)(1.0f, std::ceil(value)));
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    レンダーターゲットをビューポートサイズに合わせてリサイズする
	/////////////////////////////////////////////////////////////////////////////////////////
	void ResizeTargetToViewport(IRenderTarget* rt, const CalyxEngine::Vector2& size) {
		// 無効なレンダーターゲットまたは描画できないサイズは処理しない
		if(!rt || size.x <= 0.0f || size.y <= 0.0f) return;

		// ビューポートの論理サイズを実際のレンダーターゲットサイズへ変換する
		uint32_t	width  = ToRenderExtent(size.x);
		uint32_t	height = ToRenderExtent(size.y);
		const float scale  = (std::min)(static_cast<float>(kMaxViewportRenderWidth) / static_cast<float>(width),
										static_cast<float>(kMaxViewportRenderHeight) / static_cast<float>(height));

		// 最大解像度を超える場合は、アスペクト比を維持したまま縮小する
		if(scale < 1.0f) {
			width  = (std::max)(1u, static_cast<uint32_t>(std::floor(static_cast<float>(width) * scale)));
			height = (std::max)(1u, static_cast<uint32_t>(std::floor(static_cast<float>(height) * scale)));
		}

		// 算出した描画解像度をレンダーターゲットへ反映する
		rt->Resize(width, height);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    ゼロベクトルを考慮してベクトルを安全に正規化する
	/////////////////////////////////////////////////////////////////////////////////////////
	CalyxEngine::Vector3 SafeNormalize(
		const CalyxEngine::Vector3& value,
		const CalyxEngine::Vector3& fallback) {
		// 長さがほぼ0の場合は正規化せず、安全な代替ベクトルを返す
		if(value.LengthSquared() <= 1.0e-8f) {
			return fallback;
		}
		return value.Normalize();
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    カメラ視錐台内にビューポート用の座標軸を登録する
	/////////////////////////////////////////////////////////////////////////////////////////
	void DrawCameraViewAxis(BaseCamera* camera) {
		// 有効なカメラが存在しない場合は座標軸を登録できない
		if(!camera) return;

		// カメラのワールド行列から右・上・前方向を取得する
		const WorldTransform&		  transform = camera->GetWorldTransform();
		const CalyxEngine::Matrix4x4& world		= transform.matrix.world;
		const CalyxEngine::Vector3	  cameraPos = transform.GetWorldPosition();
		const CalyxEngine::Vector3	  right		= SafeNormalize(
			{world.m[0][0], world.m[0][1], world.m[0][2]},
			CalyxEngine::Vector3::Right());
		const CalyxEngine::Vector3 up = SafeNormalize(
			{world.m[1][0], world.m[1][1], world.m[1][2]},
			CalyxEngine::Vector3::Up());
		const CalyxEngine::Vector3 forward = SafeNormalize(
			{world.m[2][0], world.m[2][1], world.m[2][2]},
			CalyxEngine::Vector3::Forward());

		// 視錐台の左下付近に表示されるよう、FOVとアスペクト比から配置位置を求める
		const float				   distance	  = 2.0f;
		const float				   halfHeight = std::tan(camera->GetFovY() * 0.5f) * distance;
		const float				   halfWidth  = halfHeight * camera->GetAspectRatio();
		const float				   axisLength = halfHeight * 0.18f;
		const CalyxEngine::Vector3 origin =
			cameraPos +
			forward * distance -
			right * (halfWidth * 0.78f) -
			up * (halfHeight * 0.70f);

		// X・Y・Z軸をそれぞれ赤・青・緑のラインとして描画キューへ登録する
		auto* drawer = PrimitiveDrawer::GetInstance();
		drawer->DrawViewportLine3d(origin, origin + CalyxEngine::Vector3::Right() * axisLength, {1.0f, 0.15f, 0.12f, 1.0f});
		drawer->DrawViewportLine3d(origin, origin + CalyxEngine::Vector3::Up() * axisLength, {0.15f, 0.35f, 1.0f, 1.0f});
		drawer->DrawViewportLine3d(origin, origin + CalyxEngine::Vector3::Forward() * axisLength, {0.20f, 0.90f, 0.30f, 1.0f});
	}
} // namespace

namespace CalyxEngine {
	/////////////////////////////////////////////////////////////////////////////////////////
	//    SceneManagerを生成し、シーン遷移サービスを初期化する
	/////////////////////////////////////////////////////////////////////////////////////////
	SceneManager::SceneManager(DxCore* dx)
		: dx_(dx) {
		// 外部システムから受け取った遷移要求をSceneManagerへ転送するサービスを生成する
		transitionService_ = std::make_unique<SceneTransitionService>(*this);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    SceneManagerが保持するリソースを破棄する
	/////////////////////////////////////////////////////////////////////////////////////////
	SceneManager::~SceneManager() = default;

	/////////////////////////////////////////////////////////////////////////////////////////
	//    シーン遷移要求を受け付けるインターフェースを取得する
	/////////////////////////////////////////////////////////////////////////////////////////
	ISceneTransitionRequestor& SceneManager::GetTransitionRequestor() {
		return *transitionService_;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    シーン管理とエディタ描画に必要な機能を初期化する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::Initialize() {
		// 初期化処理の開始をログへ記録する
		EngineLogger::GetInstance().Add(LogLevel::Info, LogCategory::Engine, "Scene manager initialization started.", "SceneManager");
#if defined(_DEBUG) || defined(DEVELOP)
		// エディタビルドでのみ使用するピッキング・グリッド・プレビュー描画機能を生成する
		pickingPass_ = std::make_unique<PickingPass>();
		pickingPass_->Initialize(1280, 720);
		editorGridRenderer_ = std::make_unique<GridRenderer>();
		editorGridRenderer_->Initialize();
		editorPreviewModelRenderer_ = std::make_unique<ModelRenderer>();
#endif
		// すべての初期化が完了したことをログへ記録する
		EngineLogger::GetInstance().Add(LogLevel::Info, LogCategory::Engine, "Scene manager initialization completed.", "SceneManager");
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    指定されたシーンファイルを読み込み、現在のシーンとして設定する
	/////////////////////////////////////////////////////////////////////////////////////////
	bool SceneManager::OpenScene(const std::filesystem::path& scenePath) {
		// 空のパスによる読み込み要求を拒否する
		if(scenePath.empty()) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Editor, "Scene open failed because the path is empty.", "SceneManager");
			return false;
		}
		// 再生中にシーンを開いた場合、読み込み後にランタイムコンテキストを再構築する
		const bool rebuildRuntime = pPlaySession_ && pPlaySession_->IsRuntime();

		// 読み込み失敗時に復元できるよう、現在のコンテキストを退避して新規コンテキストを生成する
		SceneContext* previousContext = SceneContext::Current();
		auto		  nextContext	  = std::make_unique<SceneContext>();
		nextContext->Initialize(false);
		// シーンファイルを新しいコンテキストへデシリアライズする
		if(!SceneSerializer::Load(*nextContext, scenePath.generic_string())) {
			if(previousContext) previousContext->MakeCurrent();
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Editor, "Scene deserialization failed: " + scenePath.generic_string(), "SceneManager");
			return false;
		}
		// 読み込んだコンテキストへシーンパスと遷移要求インターフェースを設定する
		nextContext->SetScenePath(scenePath.generic_string());
		nextContext->SetSceneTransitionRequestor(&GetTransitionRequestor());

		// 現在のシーンが保持するコンテキスト依存の状態を破棄する
		if(activeScene_.scene) activeScene_.scene->OnExit();

		// 派生シーンクラスを使用せず、.sceneファイルを扱う共通BaseSceneを生成する
		activeScene_.scene = std::make_unique<BaseScene>();
		activeScene_.scene->SetSceneName(nextContext->GetSceneName());
		activeScene_.scene->SetTransitionRequestor(&GetTransitionRequestor());
		activeScene_.ctx		  = std::move(nextContext);
		activeScene_.assetsLoaded = false;

		// 別シーンの編集操作をUndo/Redoできないようコマンド履歴を破棄する
		CommandManager::GetInstance()->ClearHistory();
		// PlaySessionへ新しい編集コンテキストを通知し、必要に応じてランタイムを再構築する
		if(pPlaySession_) {
			pPlaySession_->BindEditorContext(activeScene_.ctx.get());
			if(rebuildRuntime) pPlaySession_->RebuildRuntimeFromEditor(activeScene_.ctx.get());
		}
		// コンテキストのバインド情報を無効化し、新しいシーンへ初期バインドする
		lastBoundCtx_	= nullptr;
		lastRuntimeGen_ = 0;
		RebindIfContextChanged();
		EngineLogger::GetInstance().Add(LogLevel::Info, LogCategory::Editor, "Scene opened successfully: " + scenePath.generic_string(), "SceneManager");
		return true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    シーンアセットGUIDから対象シーンを解決して開く
	/////////////////////////////////////////////////////////////////////////////////////////
	bool SceneManager::OpenScene(const Guid& sceneAssetGuid) {
		// アセットデータベースからGUIDに対応するシーンレコードを取得する
		const AssetRecord* record = AssetDatabase::GetInstance()->Get(sceneAssetGuid);
		if(!record || record->type != AssetType::Scene) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Asset, "Scene asset GUID could not be resolved: " + sceneAssetGuid.ToString(), "SceneManager");
			return false;
		}
		// 解決したソースパスを使用して通常のシーン読み込み処理へ委譲する
		return OpenScene(record->sourcePath);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    ファイルパスを指定してシーン遷移を要求する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::RequestSceneChange(const std::filesystem::path& scenePath) { RequestSceneChangeInternal(scenePath); }
	/////////////////////////////////////////////////////////////////////////////////////////
	//    シーンアセットGUIDを指定してシーン遷移を要求する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::RequestSceneChange(const Guid& sceneAssetGuid) { RequestSceneChangeInternal(sceneAssetGuid); }

	void SceneManager::RequestSceneChange(const std::filesystem::path& scenePath, std::unique_ptr<BaseSceneTransitionEffect> effect) {
		RequestSceneChangeInternal(scenePath, nullptr, std::move(effect));
	}

	void SceneManager::RequestSceneChange(const Guid& sceneAssetGuid, std::unique_ptr<BaseSceneTransitionEffect> effect) {
		RequestSceneChangeInternal(sceneAssetGuid, nullptr, std::move(effect));
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    現在編集対象として保持しているSceneContextを取得する
	/////////////////////////////////////////////////////////////////////////////////////////
	SceneContext* SceneManager::GetCurrentSceneContext() const {
		return activeScene_.ctx.get();
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    現在開いているシーンファイルのパスを取得する
	/////////////////////////////////////////////////////////////////////////////////////////
	std::filesystem::path SceneManager::GetCurrentScenePath() const {
		// コンテキストが存在しない場合は空のパスを返す
		auto* context = GetCurrentSceneContext();
		return context ? std::filesystem::path(context->GetScenePath()) : std::filesystem::path{};
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    再生状態を考慮して現在有効なSceneContextを取得する
	/////////////////////////////////////////////////////////////////////////////////////////
	SceneContext* SceneManager::ActiveCtx() const {
		// PlaySessionが存在する場合は、編集・再生状態に応じたコンテキストを優先する
		if(pPlaySession_ && pPlaySession_->IsRuntime()) return pPlaySession_->GetContext();
		if(pPlaySession_) return pPlaySession_->GetContext();
		return activeScene_.ctx.get();
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    現在有効なコンテキストがランタイム状態かを取得する
	/////////////////////////////////////////////////////////////////////////////////////////
	bool SceneManager::ActiveRuntimeFlag() const {
		if(pPlaySession_) return pPlaySession_->IsRuntime();
		return activeScene_.ctx ? activeScene_.ctx->IsRuntime() : false;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    エディタプレビューで描画するSceneContextを設定する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::SetEditorPreviewContext(SceneContext* ctx) {
		editorPreviewCtx_ = ctx;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    ランタイムとエディタを含むすべてのシーンコンテキストを破棄する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::ClearAllContexts() {
		// コンテキスト破棄の開始をログへ記録する
		EngineLogger::GetInstance().Add(LogLevel::Info, LogCategory::Engine, "Clearing all scene contexts.", "SceneManager");
		// コンテキスト切り替え検出に使用する参照と世代情報をリセットする
		editorPreviewCtx_ = nullptr;
		lastBoundCtx_	  = nullptr;
		lastRuntimeGen_	  = 0;

		// 破棄済みオブジェクトを参照する編集履歴が残らないようクリアする
		CommandManager::GetInstance()->ClearHistory();

		// PlaySessionが保持するランタイムコンテキストを先に解放する
		if(pPlaySession_) {
			pPlaySession_->ClearRuntimeContext();
		}

		// 現在のシーンを終了させ、編集コンテキスト内のオブジェクトを破棄する
		if(activeScene_.scene) activeScene_.scene->OnExit();
		if(activeScene_.ctx) activeScene_.ctx->Clear();
		activeScene_ = {};
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    現在のシーンがゲーム終了状態かを取得する
	/////////////////////////////////////////////////////////////////////////////////////////
	bool SceneManager::GetIsEndGame() const { return activeScene_.scene && activeScene_.scene->GetIsEndGame(); }

	/////////////////////////////////////////////////////////////////////////////////////////
	//    有効なSceneContextの変更を検出し、シーンを再バインドする
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::RebindIfContextChanged() {
		// 編集・再生状態を考慮した現在のコンテキストを取得する
		SceneContext* ctx = ActiveCtx();
		if(!ctx) return;

		// 同じポインタでもランタイムが再生成されている可能性があるため世代番号も比較する
		const uint64_t gen = pPlaySession_ ? pPlaySession_->RuntimeGeneration() : 0;

		// コンテキストまたはランタイム世代が変更された場合のみ再バインドする
		if(ctx != lastBoundCtx_ || gen != lastRuntimeGen_) {
			auto& slot = activeScene_;

			// 前回のコンテキストに依存するシーン内キャッシュを破棄する
			slot.scene->OnExit();

			// 新しいコンテキストをカレント化し、BaseSceneへ注入する
			ctx->MakeCurrent();
			slot.scene->InjectContext(ctx);

			// シーン共通アセットはシーンを開いてから一度だけ読み込む
			if(!slot.assetsLoaded) {
				slot.scene->LoadAssets();
				slot.assetsLoaded = true;
			}

			// 遷移元から受け取ったペイロードを新しいシーンへ一度だけ引き渡す
			if(pendingPayload_) {
				slot.scene->OnPayload(std::move(pendingPayload_));
			}

			// 新しいコンテキストに対してシーンを初期化し、開始通知を行う
			slot.scene->Initialize();
			slot.scene->OnEnter();
			// ランタイム再構築前に退避されたデバッグカメラ状態を復元する
			if(pPlaySession_) {
				pPlaySession_->ApplyPendingDebugCameraState(ctx);
			}

			// 次回の変更検出に使用するコンテキストと世代番号を保存する
			lastBoundCtx_	= ctx;
			lastRuntimeGen_ = gen;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    有効なコンテキストとシーンを更新し、保留中のシーン遷移を処理する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::Update(float dt, float alwaysDt) {
		// シーンが開かれていない場合は更新処理を行わない
		if(!activeScene_.scene) return;

		// PlaySessionから終了要求を受けた場合はランタイム終了処理を確定する
		if(pPlaySession_ && pPlaySession_->ExitRequested()) {
			pPlaySession_->FinalizeExitCleanup();
			lastBoundCtx_	= nullptr;
			lastRuntimeGen_ = 0;
		}

		// 再生開始・停止やランタイム再構築によるコンテキスト変更を反映する
		RebindIfContextChanged();

		SceneContext* ctx = ActiveCtx();
		if(!ctx) return;

		// オブジェクト管理や各システムなど、SceneContext側の更新を実行する
		ctx->MakeCurrent();
		ctx->Update(dt, alwaysDt, ActiveRuntimeFlag());

		// 同じコンテキストをBaseSceneへ注入し、シーン固有の更新処理を実行する
		auto& slot = activeScene_;
		slot.scene->InjectContext(ctx);
		slot.scene->Update(dt);

		// 更新中に登録された遷移要求をフレーム末尾で安全に反映する
		if(pendingScenePath_.has_value() && transitionPhase_ == TransitionPhase::None) {
			activeTransitionEffect_ = std::move(pendingTransitionEffect_);
			if(!activeTransitionEffect_) {
				activeTransitionEffect_ = std::make_unique<FadeBlackOutEffect>();
			}
			transitionPhase_ = TransitionPhase::FadeOut;
			activeTransitionEffect_->StartFadeOut();
		}

		if(transitionPhase_ == TransitionPhase::FadeOut) {
			activeTransitionEffect_->FadeOutUpdate(alwaysDt);
			if(activeTransitionEffect_->IsFadeOutFinished()) {
				auto nextPath = std::move(*pendingScenePath_);
				pendingScenePath_.reset();
				OpenScene(nextPath);
				activeTransitionEffect_->StartFadeIn();
				transitionPhase_ = TransitionPhase::FadeIn;
			}
		} else if(transitionPhase_ == TransitionPhase::FadeIn) {
			activeTransitionEffect_->FadeInUpdate(alwaysDt);
			if(activeTransitionEffect_->IsFadeInFinished()) {
				activeTransitionEffect_.reset();
				transitionPhase_ = TransitionPhase::None;
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    描画前にエディタプレビューと現在のシーンの後更新処理を実行する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::PostUpdate(ID3D12GraphicsCommandList* cmd, PipelineService* pso) {
		// シーンが開かれていない場合は描画前更新を行わない
		if(!activeScene_.scene) return;

		// 専用プレビューが有効な場合は、プレビュー側の描画データを先に更新する
		if(editorPreviewCtx_) {
			editorPreviewCtx_->MakeCurrent();
			editorPreviewCtx_->PostUpdate(pso, cmd);
		}

		// 現在のシーンへ戻し、アクティブなコンテキストで後更新処理を実行する
		RebindIfContextChanged();
		if(auto* ctx = ActiveCtx()) {
			ctx->MakeCurrent();
		}
		activeScene_.scene->PostUpdate(cmd, pso);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    メインビュー、デバッグビュー、アセットプレビューを描画する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso) {
		// 描画対象となるシーンが存在しない場合は処理しない
		if(!activeScene_.scene) return;
		RebindIfContextChanged();

		if(auto* ctx = ActiveCtx()) ctx->MakeCurrent();

		// メインカメラを使用してゲームビューをオフスクリーンへ描画する
		CameraManager::SetTypeStatic(CameraType::Default);
		auto* offscreen = dx_->GetRenderTargetCollection().Get("Offscreen");
		ResizeTargetToViewport(offscreen, CameraManager::GetViewportSizeStatic(ViewportType::VIEWPORT_MAIN));
		DrawForRenderTarget(offscreen, cmd, pso);
		// 設定が有効な場合はメインビューへグリッドとデバッグラインを追加描画する
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
		// メインビューのカメラ方向を確認するための座標軸を重ねて描画する
		RenderViewportAxisToRenderTarget(offscreen, cmd);

#if defined(_DEBUG) || defined(DEVELOP)
		// エディタビルドではデバッグビューまたは専用プレビューを描画する
		auto* debugRT = dx_->GetRenderTargetCollection().Get("DebugView");
		if(renderDebugView_) {
			ResizeTargetToViewport(debugRT, CameraManager::GetViewportSizeStatic(ViewportType::VIEWPORT_DEBUG));
			if(editorPreviewCtx_) {
				// Prefabなどの専用編集対象が存在する場合はプレビューコンテキストを描画する
				DrawEditorPreview(debugRT, cmd, pso);
			} else {
				// 通常時はデバッグカメラを使用して現在のシーンを描画する
				if(auto* ctx = ActiveCtx()) ctx->MakeCurrent();
				CameraManager::SetTypeStatic(CameraType::Debug);
				DrawForRenderTarget(debugRT, cmd, pso);

				// デバッグビューと同じ解像度でオブジェクトIDバッファを描画する
				if(renderPicking_ && pickingPass_ && debugRT) {
					auto vp = debugRT->GetViewport();
					pickingPass_->Resize(static_cast<int32_t>(vp.Width), static_cast<int32_t>(vp.Height));
					if(auto* renderer = activeScene_.scene->GetModelRenderer()) {
						pickingPass_->Render(cmd, renderer, pso);
					}
					debugRT->SetRenderTarget(cmd);
				}
				// 設定が有効な場合はデバッグビューへエディタグリッドを描画する
				if(EngineSettings::GetInstance()->GetData().editor.showEditorGrid && debugRT) {
					debugRT->SetRenderTarget(cmd);
					if(auto* cam = CameraManager::GetActive()) {
						if(editorGridRenderer_) {
							editorGridRenderer_->Render(cmd, pso, cam);
						}
					}
				}
				// DebugView専用プリミティブを含めてデバッグラインを描画する
				if(renderDebugLines) {
					CameraManager::SetTypeStatic(CameraType::Debug);
					RenderDebugPrimitivesToRenderTarget(debugRT, cmd, true);
				}
				// デバッグカメラの向きを示す座標軸を最後に重ねて描画する
				CameraManager::SetTypeStatic(CameraType::Debug);
				RenderViewportAxisToRenderTarget(debugRT, cmd);
			}
		}

#endif

		// 保留されているアセットサムネイルを描画し、元のSceneContextへ復帰する
		if(auto* previewRT = dx_->GetRenderTargetCollection().Get("AssetPreview")) {
			if(auto* previews = AssetPreviewManager::GetInstance()) {
				SceneContext* previousContext = SceneContext::Current();
				previews->ProcessRenderQueue(cmd, pso, previewRT, 1);
				if(previousContext) previousContext->MakeCurrent();
			}
		}

		// このフレームで登録された一時デバッグメッシュを破棄する
		PrimitiveDrawer::GetInstance()->ClearMesh();
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    エディタビューポートのデバッグ表示とピッキング描画の有効状態を設定する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::SetEditorViewportRenderState(bool renderDebugView, bool renderPicking) {
		// デバッグビューが無効な場合は、表示先のないピッキング処理も無効化する
		renderDebugView_ = renderDebugView;
		renderPicking_	 = renderDebugView && renderPicking;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    エディタプレビュー用コンテキストをデバッグビューへ描画する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::DrawEditorPreview(IRenderTarget*				rt,
										 ID3D12GraphicsCommandList* cmd,
										 PipelineService*			pso) {
		// 描画先またはプレビュー対象が存在しない場合は処理しない
		if(!rt || !editorPreviewCtx_) return;

		// プレビューコンテキストとデバッグカメラを有効化する
		editorPreviewCtx_->MakeCurrent();
		CameraManager::SetTypeStatic(CameraType::Debug);

		// プレビュー用レンダーターゲットを設定して前フレームの内容をクリアする
		rt->SetRenderTarget(cmd);
		rt->Clear(cmd);

		// オブジェクトの配置基準を確認できるようグリッドを描画する
		if(auto* cam = CameraManager::GetActive()) {
			if(editorGridRenderer_) {
				editorGridRenderer_->Render(cmd, pso, cam);
			}
		}

		if(editorPreviewModelRenderer_) {
			// 前フレームの登録内容を破棄し、今回描画するモデルの収集を開始する
			editorPreviewModelRenderer_->BeginFrame();

			// プレビューコンテキスト内の描画可能オブジェクトをモデル種別ごとに登録する
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

			// 3Dカメラが利用できる場合はカリングし、それ以外は全モデルを可視としてバッチ化する
			if(auto* camera = dynamic_cast<Camera3d*>(CameraManager::GetActive())) {
				editorPreviewModelRenderer_->PreCullAndBatch(camera, false);
			} else {
				editorPreviewModelRenderer_->BuildAllVisibleBatches();
			}
			// 作成した描画バッチをプレビュー用ライト環境で描画する
			editorPreviewModelRenderer_->DrawAll(cmd,
												 GraphicsGroup::GetInstance()->GetDevice().Get(),
												 rt,
												 pso,
												 editorPreviewCtx_->GetLightLibrary(),
												 nullptr);
		}

		// プレビューコンテキスト内のエフェクトをモデル描画後に重ねて描画する
		editorPreviewCtx_->GetFxSystem()->Render(pso, cmd);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    デバッグプリミティブを深度設定ごとにレンダーターゲットへ描画する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::RenderDebugPrimitivesToRenderTarget(IRenderTarget*			  rt,
														   ID3D12GraphicsCommandList* cmd,
														   bool						  includeDebugViewOnly) {
		// 無効な描画先・コマンド、または専用プレビュー中の場合は描画しない
		if(!rt || !cmd || editorPreviewCtx_) return;

		// デバッグプリミティブの出力先を指定する
		rt->SetRenderTarget(cmd);
		if(auto* cam = CameraManager::GetActive()) {
			// 深度テストありのラインを描画し、遮蔽物との前後関係を反映する
			GraphicsGroup::GetInstance()->SetCommand(cmd, PipelineType::Line, BlendMode::NORMAL);
			cam->SetCommand(cmd, PipelineType::Line);
			PrimitiveDrawer::GetInstance()->Render(includeDebugViewOnly, LineDepthMode::DepthTest);

			// 常に前面へ表示するラインを深度テストなしで描画する
			GraphicsGroup::GetInstance()->SetCommand(cmd, PipelineType::LineNoDepth, BlendMode::NORMAL);
			cam->SetCommand(cmd, PipelineType::LineNoDepth);
			PrimitiveDrawer::GetInstance()->Render(includeDebugViewOnly, LineDepthMode::NoDepthTest);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    カメラ基準のビューポート座標軸をレンダーターゲットへ描画する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::RenderViewportAxisToRenderTarget(IRenderTarget*			   rt,
														ID3D12GraphicsCommandList* cmd) {
		if(!rt || !cmd || editorPreviewCtx_) return;

		rt->SetRenderTarget(cmd);
		if(auto* cam = CameraManager::GetActive()) {
			// 現在のカメラ姿勢を基準に座標軸ラインを登録する
			DrawCameraViewAxis(cam);
			GraphicsGroup::GetInstance()->SetCommand(cmd, PipelineType::LineNoDepth, BlendMode::NORMAL);
			cam->SetCommand(cmd, PipelineType::LineNoDepth);
			// 座標軸を深度テストなしで描画し、登録済みラインを次フレームへ残さない
			PrimitiveDrawer::GetInstance()->RenderViewportLines();
			PrimitiveDrawer::GetInstance()->ClearViewportLines();
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    ポストエフェクト対象外のスプライトを指定レンダーターゲットへ描画する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::DrawSpritesToRenderTarget(IRenderTarget* rt, ID3D12GraphicsCommandList* cmd, PipelineService* pso, bool transitionToShaderResource) {
		// 指定されたレンダーターゲットが存在しない場合は描画しない
		if(!rt) return;

		// スプライトを書き込めるようリソース状態をRenderTargetへ遷移する
		rt->TransitionTo(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
		rt->SetRenderTarget(cmd);

		// 3Dシーンやポストエフェクトを通さず、スプライトのみを描画する
		activeScene_.scene->DrawSpritesOnly(cmd, pso);

		// 後続処理からテクスチャとして参照する出力だけShaderResource状態へ戻す
		if(transitionToShaderResource) {
			rt->TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    現在のシーンを指定されたレンダーターゲットへ描画する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::DrawForRenderTarget(IRenderTarget*			  rt,
										   ID3D12GraphicsCommandList* cmd,
										   PipelineService*			  pso) {

		// 描画先が存在しない場合は処理しない
		if(!rt) return;

		// 描画先を設定し、前フレームのカラー・深度情報をクリアする
		rt->SetRenderTarget(cmd);
		rt->Clear(cmd);

		// 現在のシーンへ描画先を渡して通常描画を実行する
		auto& slot = activeScene_;
		slot.scene->Draw(cmd, pso, rt);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    ポストエフェクトの影響を受けないスプライトを最終出力へ描画する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::DrawNotAffectedFromPE(ID3D12GraphicsCommandList* cmd, PipelineService* pso) {
		// 描画対象となるシーンが存在しない場合は処理しない
		if(!activeScene_.scene) return;

		// ポストエフェクト完了後の出力へUIスプライトを合成する
		auto* postOutput = dx_->GetRenderTargetCollection().Get("PostEffectOutput");
		DrawSpritesToRenderTarget(postOutput, cmd, pso, false);
		if(activeTransitionEffect_ && postOutput) {
			// The editor game viewport displays PostEffectOutput as an ImGui texture.
			// Draw the transition here as well as on the final back buffer so it is
			// visible in both editor play mode and standalone builds.
			activeTransitionEffect_->Draw(cmd, pso);
		}
		if(postOutput) {
			postOutput->TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		// 最終バックバッファにも同じスプライトを描画し、画面へ出力する
		auto* backBuffer = dx_->GetRenderTargetCollection().Get("BackBuffer");
		DrawSpritesToRenderTarget(backBuffer, cmd, pso, false);
		if(activeTransitionEffect_) {
			activeTransitionEffect_->Draw(cmd, pso);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    ファイルパスとペイロードをシーン遷移要求としてキューへ登録する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::RequestSceneChangeInternal(const std::filesystem::path& scenePath, std::unique_ptr<IScenePayload> payload) {
		// 遷移先を特定できない空パスの要求はキューへ登録しない
		if(scenePath.empty()) {
			EngineLogger::GetInstance().Add(LogLevel::Warning, LogCategory::Game, "Scene change request ignored because the scene path is empty.", "SceneManager");
			return;
		}
		// 更新中のシーンを即座に破棄しないよう、ペイロードと遷移先を保留領域へ保存する
		pendingPayload_	  = std::move(payload);
		pendingScenePath_ = scenePath;
		pendingTransitionEffect_.reset();
		EngineLogger::GetInstance().Add(LogLevel::Trace, LogCategory::Game, "Scene change queued: " + scenePath.generic_string(), "SceneManager");
	}

	void SceneManager::RequestSceneChangeInternal(const std::filesystem::path& scenePath,
		std::unique_ptr<IScenePayload> payload,
		std::unique_ptr<BaseSceneTransitionEffect> effect) {
		RequestSceneChangeInternal(scenePath, std::move(payload));
		if(pendingScenePath_.has_value()) {
			pendingTransitionEffect_ = std::move(effect);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//    シーンアセットGUIDを解決し、ペイロード付きの遷移要求を登録する
	/////////////////////////////////////////////////////////////////////////////////////////
	void SceneManager::RequestSceneChangeInternal(const Guid& sceneAssetGuid, std::unique_ptr<IScenePayload> payload) {
		// アセットデータベースからGUIDに対応するシーンファイルを解決する
		const AssetRecord* record = AssetDatabase::GetInstance()->Get(sceneAssetGuid);
		if(!record || record->type != AssetType::Scene) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Asset, "Scene transition GUID could not be resolved: " + sceneAssetGuid.ToString(), "SceneManager");
			return;
		}
		// 解決したパスを使用して共通の遷移キュー登録処理へ委譲する
		RequestSceneChangeInternal(record->sourcePath, std::move(payload));
	}

	void SceneManager::RequestSceneChangeInternal(const Guid& sceneAssetGuid,
		std::unique_ptr<IScenePayload> payload,
		std::unique_ptr<BaseSceneTransitionEffect> effect) {
		const AssetRecord* record = AssetDatabase::GetInstance()->Get(sceneAssetGuid);
		if(!record || record->type != AssetType::Scene) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Asset, "Scene transition GUID could not be resolved: " + sceneAssetGuid.ToString(), "SceneManager");
			return;
		}
		RequestSceneChangeInternal(record->sourcePath, std::move(payload), std::move(effect));
	}

} // namespace CalyxEngine
