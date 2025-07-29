#include "CameraManager.h"
#include <Engine/Scene/Utility/SceneUtility.h>

static CameraManager* CurrMgr(){ auto* ctx = SceneContext::Current(); return ctx ? ctx->    () : nullptr; }

void CameraManager::Initialize(SceneContext*){
    camera3d_ = SceneAPI::Instantiate<Camera3d>("MainCamera");
    debugCamera_ = SceneAPI::Instantiate<DebugCamera>("DebugCamera");
    cameras_[CameraType::Default] = camera3d_.get();
    cameras_[CameraType::Debug] = debugCamera_.get();
}
void CameraManager::Update([[maybe_unused]]float){}
void CameraManager::TransferToGPU(){ for (auto& kv : cameras_) kv.second->TransfarToGPU(); }
void CameraManager::Finalize(){ camera3d_.reset(); debugCamera_.reset(); }
void CameraManager::SetType(CameraType t){ type_ = t; for (auto& kv : cameras_) kv.second->SetActive(kv.first == t); }
void CameraManager::SetViewportSize(ViewportType vt, const Vector2& sz){ if (vt == ViewportType::VIEWPORT_MAIN) mainVp_ = sz; else if (vt == ViewportType::VIEWPORT_DEBUG) debugVp_ = sz; }
Vector2 CameraManager::GetViewportSize(ViewportType vt) const{ return vt == ViewportType::VIEWPORT_MAIN ? mainVp_ : vt == ViewportType::VIEWPORT_DEBUG ? debugVp_ : Vector2 {0,0}; }
void CameraManager::SetAspectRatio(float w, float h){ if (h > 0){ float asp = w / h; for (auto& kv : cameras_) kv.second->SetAspectRatio(asp); } }

/* 静的ラッパ */
Camera3d* CameraManager::GetMain3d(){ return CurrMgr() ? CurrMgr()->GetCamera3d() : nullptr; }
DebugCamera* CameraManager::GetDebug(){ return CurrMgr() ? CurrMgr()->GetDebugCamera() : nullptr; }
BaseCamera* CameraManager::GetActive(){ return CurrMgr() ? CurrMgr()->GetActiveCamera() : nullptr; }
void CameraManager::SetTypeStatic(CameraType t){ if (CurrMgr()) CurrMgr()->SetType(t); }
void CameraManager::SetViewportSizeStatic(ViewportType vt, const Vector2& sz){ if (CurrMgr()) CurrMgr()->SetViewportSize(vt, sz); }
