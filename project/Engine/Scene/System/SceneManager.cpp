#include "SceneManager.h"
/* ======================================================================== */
/* include space                                                            */
/* ======================================================================== */
#include <Engine/Application/UI/Panels/HierarchyPanel.h>
#include <Engine/Application/UI/Panels/SceneSwitcherPanel.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Scene/System/SceneFactory.h>

SceneManager::SceneManager(DxCore* dxCore) : pDxCore_(dxCore) {
	for (int i = 0; i < static_cast<int>(SceneType::count); ++i) {
		auto st = static_cast<SceneType>(i);

		/* Scene 本体 */
		slots_[i].scene = SceneFactory::CreateScene(st);
		slots_[i].scene->SetTransitionRequestor(this);

		/* SceneContext */
		slots_[i].ctx = std::make_unique<SceneContext>();
		slots_[i].ctx->Initialize();

		/* 注入 */
		slots_[i].scene->InjectContext(slots_[i].ctx.get());
	}

#ifdef _DEBUG
	currentSceneNo_ = static_cast<int>(SceneType::PLAY);
#endif
	nextSceneNo_ = currentSceneNo_;
}

SceneManager::~SceneManager() = default;

void SceneManager::Initialize() {
#ifdef _DEBUG
	if (pEngineUI_) {
		auto switcher = std::make_unique<SceneSwitcherPanel>(this);
		switcher->AddSceneOption("Game Scene", SceneType::PLAY);
		switcher->AddSceneOption("Test Scene", SceneType::TEST);
		pEngineUI_->AddPanel(std::move(switcher));
	}
#endif

	slots_[currentSceneNo_].ctx->MakeCurrent();
	slots_[currentSceneNo_].scene->Initialize();

#ifdef _DEBUG
	if (pEngineUI_)
		pEngineUI_->GetHierarchyPanel()->SetSceneObjectLibrary(
			slots_[currentSceneNo_].ctx->GetObjectLibrary());
#endif
}

void SceneManager::Update(float dt) {
	if (currentSceneNo_ != nextSceneNo_)
		SwitchScene(nextSceneNo_);
	slots_[currentSceneNo_].scene->Update(dt);
}

void SceneManager::SwitchScene(int newNo) {
	slots_[currentSceneNo_].scene->CleanUp();

	currentSceneNo_ = newNo;
	slots_[currentSceneNo_].ctx->MakeCurrent();
	slots_[currentSceneNo_].scene->Initialize();

#ifdef _DEBUG
	if (pEngineUI_)
		pEngineUI_->GetHierarchyPanel()->SetSceneObjectLibrary(
			slots_[currentSceneNo_].ctx->GetObjectLibrary());
#endif
}

void SceneManager::PostUpdate(ID3D12GraphicsCommandList* cmd,
							  PipelineService* pso) {
	slots_[currentSceneNo_].scene->PostUpdate(cmd, pso);
}

void SceneManager::Draw(ID3D12GraphicsCommandList* cmd,
						PipelineService* pso) {
	/* Game view (Default camera) */
	CameraManager::SetTypeStatic(CameraType::Default);
	auto* offscreen = pDxCore_->GetRenderTargetCollection().Get("Offscreen");
	DrawForRenderTarget(offscreen, cmd, pso);

#ifdef _DEBUG
	/* Debug view (Debug camera) */
	CameraManager::SetTypeStatic(CameraType::Debug);
	auto* debugRT = pDxCore_->GetRenderTargetCollection().Get("DebugView");
	DrawForRenderTarget(debugRT, cmd, pso);
#endif

	/* Primitive lines */
	GraphicsGroup::GetInstance()->SetCommand(cmd, PipelineType::Line, BlendMode::NORMAL);
	if (auto* cam = CameraManager::GetActive())
		cam->SetCommand(cmd, PipelineType::Line);
	PrimitiveDrawer::GetInstance()->Render();
	PrimitiveDrawer::GetInstance()->ClearMesh();
}

void SceneManager::DrawForRenderTarget(IRenderTarget* rt,
									   ID3D12GraphicsCommandList* cmd,
									   PipelineService* pso) {
	rt->SetRenderTarget(cmd);
	rt->Clear(cmd);
	slots_[currentSceneNo_].scene->Draw(cmd, pso, rt->GetRenderTargetType());
}

void SceneManager::DrawNotAffectedFromPE(ID3D12GraphicsCommandList* cmd,
										 PipelineService* pso) {
	auto* backbuffer = pDxCore_->GetRenderTargetCollection().Get("BackBuffer");
	backbuffer->SetRenderTarget(cmd);
	slots_[currentSceneNo_].scene->DrawSpritesOnly(cmd, pso);
}

void SceneManager::RequestSceneChange(SceneType next) {
	nextSceneNo_ = static_cast<int>(next);
}

SceneContext* SceneManager::GetCurrentSceneContext() const {
	return slots_[currentSceneNo_].ctx.get();
}
