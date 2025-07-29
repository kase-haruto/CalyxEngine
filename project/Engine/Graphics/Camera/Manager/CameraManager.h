#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/graphics/camera/3d/FollowCamera.h>
#include <Engine/graphics/camera/3d/DebugCamera.h>
#include <Engine/graphics/camera/Base/BaseCamera.h>
#include <Engine/Graphics/Camera/Viewport/ViewportDetail.h>

//* c++ *//
#include <memory>
#include <unordered_map>

/* ========================================================================
/*	enum
/* ===================================================================== */
enum class CameraType{
	Default,
	Debug,
};
class SceneContext; // fwd

class CameraManager{
public:
	/* シーン所有側が呼ぶ */
	void Initialize(SceneContext* owner);
	void Update(float dt);
	void TransferToGPU();
	void Finalize();

	/* 非 static API */
	Camera3d* GetCamera3d(){ return camera3d_.get(); }
	DebugCamera* GetDebugCamera(){ return debugCamera_.get(); }
	BaseCamera* GetActiveCamera(){ return cameras_[type_]; }

	void SetType(CameraType t);
	void SetViewportSize(ViewportType, const Vector2&);
	Vector2 GetViewportSize(ViewportType) const;
	void SetAspectRatio(float w, float h);
	void Shake(float dur, float inten){ cameras_[type_]->StartShake(dur, inten); }

	/* 旧互換: 現在の SceneContext から取得して返す */
	static Camera3d* GetMain3d();
	static DebugCamera* GetDebug();
	static BaseCamera* GetActive();
	static void        SetTypeStatic(CameraType);
	static void        SetViewportSizeStatic(ViewportType, const Vector2&);
private:
	CameraType type_ = CameraType::Default;
	Vector2 mainVp_ {1920,1080};
	Vector2 debugVp_ {800,600};
	std::shared_ptr<Camera3d>    camera3d_;
	std::shared_ptr<DebugCamera> debugCamera_;
	std::unordered_map<CameraType, BaseCamera*> cameras_;
};