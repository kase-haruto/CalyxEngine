#include "CameraManager.h"
#include <Engine/Scene/Utility/SceneUtility.h>

static CameraManager* Mgr(){ 
	auto* ctx = SceneContext::Current();
	if (!ctx) return nullptr;
	return ctx->GetCameraMgr(); 
}

void CameraManager::Initialize(SceneContext*){
	main_ = SceneAPI::Instantiate<Camera3d>("MainCamera");
	debug_ = SceneAPI::Instantiate<DebugCamera>("DebugCamera");
	cameras_[CameraType::Default] = main_.get();
	cameras_[CameraType::Debug] = debug_.get();
}
void CameraManager::Update(float){ TransferToGPU(); }
void CameraManager::TransferToGPU() {
	for (auto& kv : cameras_) kv.second->TransfarToGPU();
}

void CameraManager::SetType(CameraType t) {
	active_ = t;
	for (auto& kv : cameras_) {
		if (kv.second) {
			kv.second->SetActive(kv.first == t);
		}
	}
}

const CalyxMath::Vector2& CameraManager::ViewportSize(ViewportType vt) const{
	return (vt == ViewportType::VIEWPORT_MAIN) ? vpMain_ : vpDebug_;
}
void CameraManager::SetViewportSize(ViewportType vt, const CalyxMath::Vector2& s){ (vt == ViewportType::VIEWPORT_MAIN ? vpMain_ : vpDebug_) = s; }

void CameraManager::SetAspectRatio(float w, float h){ if (h > 0){ float asp = w / h; for (auto& kv : cameras_) kv.second->SetAspectRatio(asp); } }

//------------------------------------------------------------------
// static wrappers
Camera3d*				  CameraManager::GetMain3d() { return Mgr() ? Mgr()->Main3D() : nullptr; }
std::shared_ptr<Camera3d> CameraManager::GetMain3dShared() {return Mgr() ? Mgr()->Main3DShared() : nullptr; }
DebugCamera* CameraManager::GetDebug(){ return Mgr() ? Mgr()->DebugCam() : nullptr; }
BaseCamera* CameraManager::GetActive(){ 
	auto* mgr = Mgr();
	if (!mgr) return nullptr;
	auto* active = mgr->Active();
	if (active) return active;
	return mgr->Main3D(); // Fallback to main
}
void CameraManager::SetTypeStatic(CameraType t){ if (Mgr()) Mgr()->SetType(t); }
const CalyxMath::Vector2& CameraManager::GetViewportSizeStatic(ViewportType vt){ 
	static CalyxMath::Vector2 dummy {0,0}; return Mgr() ? Mgr()->ViewportSize(vt) : dummy; }
void CameraManager::SetViewportSizeStatic(ViewportType vt, const CalyxMath::Vector2& s){ if (Mgr()) Mgr()->SetViewportSize(vt, s); }
void CameraManager::Finalize(){ 
	// static な Mgr() を経由してインスタンスを破棄するのではなく、
	// シーン終了時に SceneContext と共に破棄されるのが想定。
	// ここでの Mgr()->Finalize() は再帰呼び出しになるため削除。
}
//=============================================================