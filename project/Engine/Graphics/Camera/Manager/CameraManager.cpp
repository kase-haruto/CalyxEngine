#include "CameraManager.h"
#include <Engine/Scene/Utility/SceneUtility.h>

static CameraManager* Mgr(){ if (auto* ctx = SceneContext::Current()) return ctx->GetCameraMgr(); return nullptr; }

void CameraManager::Initialize(SceneContext*){
	main_ = SceneAPI::Instantiate<Camera3d>("MainCamera");
	debug_ = SceneAPI::Instantiate<DebugCamera>("DebugCamera");
	cameras_[CameraType::Default] = main_.get();
	cameras_[CameraType::Debug] = debug_.get();
}
void CameraManager::Update(float){}
void CameraManager::TransferToGPU(){ for (auto& kv : cameras_) kv.second->TransfarToGPU(); }

void CameraManager::SetType(CameraType t){ active_ = t; for (auto& kv : cameras_) kv.second->SetActive(kv.first == t); }

const CxMath::Vector2& CameraManager::ViewportSize(ViewportType vt) const{
	return (vt == ViewportType::VIEWPORT_MAIN) ? vpMain_ : vpDebug_;
}
void CameraManager::SetViewportSize(ViewportType vt, const CxMath::Vector2& s){ (vt == ViewportType::VIEWPORT_MAIN ? vpMain_ : vpDebug_) = s; }

void CameraManager::SetAspectRatio(float w, float h){ if (h > 0){ float asp = w / h; for (auto& kv : cameras_) kv.second->SetAspectRatio(asp); } }

//------------------------------------------------------------------
// static wrappers
Camera3d*				  CameraManager::GetMain3d() { return Mgr() ? Mgr()->Main3D() : nullptr; }
std::shared_ptr<Camera3d> CameraManager::GetMain3dShared() {return Mgr() ? Mgr()->Main3DShared() : nullptr; }
DebugCamera* CameraManager::GetDebug(){ return Mgr() ? Mgr()->DebugCam() : nullptr; }
BaseCamera* CameraManager::GetActive(){ return Mgr() ? Mgr()->Active() : nullptr; }
void CameraManager::SetTypeStatic(CameraType t){ if (Mgr()) Mgr()->SetType(t); }
const CxMath::Vector2& CameraManager::GetViewportSizeStatic(ViewportType vt){ 
	static CxMath::Vector2 dummy {0,0}; return Mgr() ? Mgr()->ViewportSize(vt) : dummy; }
void CameraManager::SetViewportSizeStatic(ViewportType vt, const CxMath::Vector2& s){ if (Mgr()) Mgr()->SetViewportSize(vt, s); }
void CameraManager::Finalize(){ if (Mgr()) Mgr()->Finalize(); }
//=============================================================
