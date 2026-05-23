#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/graphics/camera/3d/DebugCamera.h>
#include <Engine/graphics/camera/Base/BaseCamera.h>
#include <Engine/Graphics/Camera/Viewport/ViewportDetail.h>

//* c++ *//
#include <memory>
#include <unordered_map>

/* ========================================================================
/*	enum
/* ===================================================================== */
class SceneContext; // fwd

enum class CameraType{ Default, Debug };

/*-----------------------------------------------------------------------------------------
 * CameraManager
 * - カメラ管理クラス
 * - メインカメラ・デバッグカメラの切り替えとビューポートサイズの管理を担当
 *---------------------------------------------------------------------------------------*/
class CameraManager{
public:
	//――― Scene‑side lifecycle ―――――――――――――――――――――――――――――
	void Initialize(SceneContext* owner);
	void Update(float dt);
	void TransferToGPU();
	static void Finalize();

	//――― Non‑static API ――――――――――――――――――――――――――――――――――
	Camera3d* Main3D(){ return main_.get(); }
	DebugCamera* DebugCam(){ return debug_.get(); }
	BaseCamera* Active(){ return cameras_[active_]; }
	std::shared_ptr<Camera3d> Main3DShared(){ return main_; };
	void  SetType(CameraType t);
	const CalyxEngine::Vector2& ViewportSize(ViewportType) const; // const ref (cheap & safe)
	void  SetViewportSize(ViewportType, const CalyxEngine::Vector2&);
	void  SetAspectRatio(float w, float h);
	void  Shake(float d, float i){ Active()->StartShake(d, i); }

	//――― Thin static wrappers (legacy code support) ―――――――――
	static Camera3d* GetMain3d();
	static std::shared_ptr<Camera3d> GetMain3dShared();
	static DebugCamera* GetDebug();
	static BaseCamera* GetActive();
	static void SetTypeStatic(CameraType);
	static const CalyxEngine::Vector2& GetViewportSizeStatic(ViewportType);
	static void SetViewportSizeStatic(ViewportType, const CalyxEngine::Vector2&);

	void SetMainCamera(const std::shared_ptr<Camera3d>& cam) { main_ = cam; cameras_[CameraType::Default] = cam.get(); }
	void SetDebugCamera(const std::shared_ptr<DebugCamera>& cam) { debug_ = cam; cameras_[CameraType::Debug] = cam.get(); }

private:
	CameraType active_ = CameraType::Default;
	CalyxEngine::Vector2 vpMain_ {1280,720}, vpDebug_ {1280,720};

	std::shared_ptr<Camera3d>   main_;
	std::shared_ptr<DebugCamera>debug_;
	std::unordered_map<CameraType, BaseCamera*> cameras_;
};