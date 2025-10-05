#include "SceneManager.h"

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Graphics/RenderTarget/Collection/RenderTargetCollection.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Scene/Base/IScene.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Application/System/PlaySession.h>

// scene
#include <Engine/Scene/Title/TitleScene.h>
#include <Engine/Scene/Game/GameScene.h>
#include "Engine/Scene/Test/TestScene.h"

SceneManager::SceneManager(DxCore* dx) : dx_(dx){}
SceneManager::~SceneManager() = default;

//------------------------------------------------------------
void SceneManager::Initialize(){
	// 登録（この段階では Initialize しない）
	AddScene(SceneType::TITLE,std::make_unique<TitleScene>());
	AddScene(SceneType::PLAY,std::make_unique<GameScene>());
	AddScene(SceneType::TEST,std::make_unique<TestScene>());

	// 最初のシーンへ（ここで初期化が走る）
	SetCurrent(typeToIndex_.at(SceneType::TITLE));
}

//------------------------------------------------------------
size_t SceneManager::AddScene(SceneType type, std::unique_ptr<BaseScene> scene){
	SceneSlot slot;
	slot.scene = std::move(scene);
	slot.ctx = std::make_unique<SceneContext>();
	slot.ctx->Initialize(true);

	slots_.push_back(std::move(slot));
	const size_t index = slots_.size() - 1;

	typeToIndex_[type] = index;

	// 遷移リクエスト先を注入
	slots_[index].scene->SetTransitionRequestor(this);

	return index;
}

//------------------------------------------------------------
void SceneManager::SetCurrent(size_t index){
	if (index >= slots_.size()) return;

	if (pPlaySession_ && pPlaySession_->ExitRequested()){ pPlaySession_->FinalizeExitCleanup(); }

	if (!slots_.empty()){ slots_[currentIdx_].scene->OnExit(); }

	currentIdx_ = index;
	auto& s = slots_[currentIdx_];

	// 新しい Editor ctx を PlaySession に通知
	if (pPlaySession_) pPlaySession_->BindEditorContext(s.ctx.get());

	// 再生中なら新しい Editor 内容から Runtime を再構築
	if (pPlaySession_ && pPlaySession_->IsRuntime()){ pPlaySession_->RebuildRuntimeFromEditor(s.ctx.get()); }

	RebindIfContextChanged();
}

//------------------------------------------------------------
SceneContext* SceneManager::GetCurrentSceneContext() const{
	if (slots_.empty()) return nullptr;
	return slots_[currentIdx_].ctx.get();
}

//------------------------------------------------------------
SceneContext* SceneManager::ActiveCtx() const{
	if (pPlaySession_) return pPlaySession_->GetContext();
	if (slots_.empty()) return nullptr;
	return slots_[currentIdx_].ctx.get();
}

bool SceneManager::ActiveRuntimeFlag() const{
	if (pPlaySession_) return pPlaySession_->IsRuntime();
	if (slots_.empty()) return false;
	return slots_[currentIdx_].ctx->IsRuntime();
}

bool SceneManager::GetIsEndGame() const {
	return slots_[currentIdx_].scene->GetIsEndGame();
}

void SceneManager::RebindIfContextChanged(){
	SceneContext* ctx = ActiveCtx();
	if (!ctx) return;

	const uint64_t gen = pPlaySession_ ? pPlaySession_->RuntimeGeneration() : 0;

	if (ctx != lastBoundCtx_ || gen != lastRuntimeGen_){
		auto& slot = slots_[currentIdx_];

		// 前回のctxにぶら下がるキャッシュを捨てる
		slot.scene->OnExit();

		ctx->MakeCurrent();
		slot.scene->InjectContext(ctx);

		if (!slot.assetsLoaded){
			slot.scene->LoadAssets();
			slot.assetsLoaded = true;
		}

		// 毎回初期化
		slot.scene->Initialize();
		slot.scene->OnEnter();

		lastBoundCtx_ = ctx;
		lastRuntimeGen_ = gen;
	}
}

void SceneManager::Update(float dt){
	if (slots_.empty()) return;

	if (pPlaySession_ && pPlaySession_->ExitRequested()){
		pPlaySession_->FinalizeExitCleanup();
		lastBoundCtx_   = nullptr;
		lastRuntimeGen_ = 0;
	}

	RebindIfContextChanged();

	SceneContext* ctx = ActiveCtx();
	if (!ctx) return;

	ctx->MakeCurrent();
	ctx->Update(dt,ActiveRuntimeFlag());

	auto& slot = slots_[currentIdx_];
	slot.scene->InjectContext(ctx);
	slot.scene->Update(dt);

	if (pendingSwitchIndex_.has_value()){
		SetCurrent(*pendingSwitchIndex_);
		pendingSwitchIndex_.reset();
	}
}

//------------------------------------------------------------
void SceneManager::PostUpdate(ID3D12GraphicsCommandList* cmd, PipelineService* pso){
	if (slots_.empty()) return;

	RebindIfContextChanged();
	if (auto* ctx = ActiveCtx()){ 
		ctx->MakeCurrent();
	}
	slots_[currentIdx_].scene->PostUpdate(cmd, pso);
}
//------------------------------------------------------------
void SceneManager::Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso){
	if (slots_.empty()) return;
	RebindIfContextChanged();  

	if (auto* ctx = ActiveCtx()) ctx->MakeCurrent();

	CameraManager::SetTypeStatic(CameraType::Default);
	auto* offscreen = dx_->GetRenderTargetCollection().Get("Offscreen");
	DrawForRenderTarget(offscreen,cmd,pso);

#if defined(_DEBUG) || defined(DEVELOP)
	if (auto* ctx = ActiveCtx()) ctx->MakeCurrent();
	CameraManager::SetTypeStatic(CameraType::Debug);
	auto* debugRT = dx_->GetRenderTargetCollection().Get("DebugView");
	DrawForRenderTarget(debugRT,cmd,pso);
#endif

	GraphicsGroup::GetInstance()->SetCommand(cmd,PipelineType::Line,BlendMode::NORMAL);
	if (auto* cam = CameraManager::GetActive()) cam->SetCommand(cmd,PipelineType::Line);
	PrimitiveDrawer::GetInstance()->Render();
	PrimitiveDrawer::GetInstance()->ClearMesh();
}

//------------------------------------------------------------
void SceneManager::DrawForRenderTarget(IRenderTarget* rt,
	ID3D12GraphicsCommandList* cmd,
	PipelineService* pso){
	if (!rt) return;
	rt->SetRenderTarget(cmd);
	rt->Clear(cmd);

	slots_[currentIdx_].scene->Draw(cmd,pso,rt->GetRenderTargetType());
}

//------------------------------------------------------------
void SceneManager::DrawNotAffectedFromPE(ID3D12GraphicsCommandList* cmd, PipelineService* pso){
	if (slots_.empty()) return;
	slots_[currentIdx_].scene->DrawSpritesOnly(cmd,pso);
}

//------------------------------------------------------------
void SceneManager::RequestSceneChange(SceneType nextScene){
	auto it = typeToIndex_.find(nextScene);
	if (it == typeToIndex_.end()) return;
	pendingSwitchIndex_ = it->second;
}
