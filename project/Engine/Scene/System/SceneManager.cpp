#include "SceneManager.h"
/* ======================================================================== */
/* include space                                                            */
/* ======================================================================== */
// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Graphics/Pipeline/Presets/PipelinePresets.h>
#include <Engine/Graphics/RenderTarget/Collection/RenderTargetCollection.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Scene/Base/IScene.h>
#include <Engine/Scene/Game/GameScene.h>
#include <Engine/Scene/Context/SceneContext.h>

SceneManager::SceneManager(DxCore* dx) : dx_(dx){}
SceneManager::~SceneManager() = default;

//------------------------------------------------------------
void SceneManager::Initialize(){
	//ゲームシーンを作成
	AddScene(std::make_unique<GameScene>());
}

//------------------------------------------------------------
size_t SceneManager::AddScene(std::unique_ptr<BaseScene> scene){
	SceneSlot slot;
	slot.scene = std::move(scene);

	slot.ctx = std::make_unique<SceneContext>();
	slot.ctx->Initialize(true);

	slot.scene->InjectContext(slot.ctx.get());
	slot.scene->LoadAssets();
	slot.scene->Initialize();

	slots_.push_back(std::move(slot));
	return slots_.size() - 1;
}

//------------------------------------------------------------
void SceneManager::SetCurrent(size_t index){
	if (index < slots_.size()) currentIdx_ = index;
}

//------------------------------------------------------------
SceneContext* SceneManager::GetCurrentSceneContext() const{
	if (slots_.empty()) return nullptr;
	return slots_[currentIdx_].ctx.get();
}

//------------------------------------------------------------
void SceneManager::Update(float dt){
	if (slots_.empty()) return;

	auto& slot = slots_[currentIdx_];

	SceneContext* active = SceneContext::Current();
	if (!active) active = slot.ctx.get();

	slot.scene->InjectContext(active);
	slot.scene->Update(dt);
}

//------------------------------------------------------------
void SceneManager::PostUpdate(ID3D12GraphicsCommandList* cmd, PipelineService* pso){
	if (slots_.empty()) return;
	slots_[currentIdx_].scene->PostUpdate(cmd, pso);
}

//------------------------------------------------------------
//------------------------------------------------------------
void SceneManager::Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso){
	if (slots_.empty()) return;

	// ───────── Game View (Offscreen, default camera) ─────────
	CameraManager::SetTypeStatic(CameraType::Default);
	auto* offscreen = dx_->GetRenderTargetCollection().Get("Offscreen");
	DrawForRenderTarget(offscreen, cmd, pso);

#ifdef _DEBUG
	// ───────── Debug View (Debug camera) ─────────────────────
	CameraManager::SetTypeStatic(CameraType::Debug);
	auto* debugRT = dx_->GetRenderTargetCollection().Get("DebugView");
	DrawForRenderTarget(debugRT, cmd, pso);
#endif

	// ───────── Primitive lines ───────────────────────────────
	GraphicsGroup::GetInstance()->SetCommand(cmd, PipelineType::Line, BlendMode::NORMAL);
	if (auto* cam = CameraManager::GetActive())
		cam->SetCommand(cmd, PipelineType::Line);
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

	slots_[currentIdx_].scene->Draw(cmd, pso, rt->GetRenderTargetType());
}

//------------------------------------------------------------
void SceneManager::DrawNotAffectedFromPE(ID3D12GraphicsCommandList* cmd, PipelineService* pso){
	if (slots_.empty()) return;
	slots_[currentIdx_].scene->DrawSpritesOnly(cmd, pso);
}
