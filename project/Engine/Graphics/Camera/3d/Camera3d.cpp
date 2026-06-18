#include "Camera3d.h"
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Objects/3D/Geometory/AABB.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

// math / util
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Utility/Func/CxUtils.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>

// imgui
#include <externals/imgui/imgui.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

// c++
#include "Engine/Application/UI/Panels/InspectorPanel.h"
#include "Engine/Foundation/Math/MathUtil.h"

#include <cmath>

namespace {

// 列ベース: 左手系行列 M の回転部分から Right/Up/Forward を正規化して取り出す（スケール除去）
void ExtractBasisNoScale(const CalyxEngine::Matrix4x4& M,CalyxEngine::Vector3& R,CalyxEngine::Vector3& U,CalyxEngine::Vector3& F) {
	R = CalyxEngine::Vector3(M.m[0][0],M.m[0][1],M.m[0][2]).Normalize(); // +X
	U = CalyxEngine::Vector3(M.m[1][0],M.m[1][1],M.m[1][2]).Normalize(); // +Y
	F = CalyxEngine::Vector3(M.m[2][0],M.m[2][1],M.m[2][2]).Normalize(); // +Z (LH: forward)
}

// 位置のアフィン変換（点）
CalyxEngine::Vector3 TransformPoint(const CalyxEngine::Matrix4x4& M,const CalyxEngine::Vector3& p) {
	return {
			M.m[0][0] * p.x + M.m[1][0] * p.y + M.m[2][0] * p.z + M.m[3][0],
			M.m[0][1] * p.x + M.m[1][1] * p.y + M.m[2][1] * p.z + M.m[3][1],
			M.m[0][2] * p.x + M.m[1][2] * p.y + M.m[2][2] * p.z + M.m[3][2],
		};
}

// 方向のアフィン変換（ベクトル：平行移動なし）
CalyxEngine::Vector3 TransformDirection(const CalyxEngine::Matrix4x4& M,const CalyxEngine::Vector3& v) {
	return {
			M.m[0][0] * v.x + M.m[1][0] * v.y + M.m[2][0] * v.z,
			M.m[0][1] * v.x + M.m[1][1] * v.y + M.m[2][1] * v.z,
			M.m[0][2] * v.x + M.m[1][2] * v.y + M.m[2][2] * v.z,
		};
}

// 回転行列だけを正規直交化（親にスケールが乗っていても回転を復元）
CalyxEngine::Matrix4x4 OrthonormalizeRotation(const CalyxEngine::Matrix4x4& M) {
	CalyxEngine::Vector3 R,U,F;
	ExtractBasisNoScale(M,R,U,F);
	// 再直交化（Gram-Schmidt 簡易）
	R = R.Normalize();
	U = (U - R * CalyxEngine::Vector3::Dot(R,U)).Normalize();
	F = CalyxEngine::Vector3::Cross(R,U).Normalize(); // LHなら R×U=F でOK

	CalyxEngine::Matrix4x4 Rm{};
	Rm.m[0][0] = R.x;
	Rm.m[0][1] = R.y;
	Rm.m[0][2] = R.z;
	Rm.m[0][3] = 0;
	Rm.m[1][0] = U.x;
	Rm.m[1][1] = U.y;
	Rm.m[1][2] = U.z;
	Rm.m[1][3] = 0;
	Rm.m[2][0] = F.x;
	Rm.m[2][1] = F.y;
	Rm.m[2][2] = F.z;
	Rm.m[2][3] = 0;
	Rm.m[3][0] = 0;
	Rm.m[3][1] = 0;
	Rm.m[3][2] = 0;
	Rm.m[3][3] = 1;
	return Rm;
}

} // namespace

//--------------------------------- ctor ---------------------------------
Camera3d::Camera3d() : BaseCamera() {
	BaseCamera::SetName("MainCamera");
	worldTransform_.translation = {0.0f,2.0f,-3.0f};
}

Camera3d::Camera3d(const std::string& name) { SceneObject::SetName(name,ObjectType::Camera); }

float Camera3d::ExpLerpAlpha(float dt,float tau) {
	if(tau <= 1e-6f) return 1.0f;
	return 1.0f - std::exp(-dt / tau);
}

CalyxEngine::Vector3 Camera3d::SmoothDampVec(const CalyxEngine::Vector3& current,const CalyxEngine::Vector3& target,
								CalyxEngine::Vector3&       vel,float              smoothTime,float dt) {
	// Unity の SmoothDamp 近似
	float st    = (std::max)(0.0001f,smoothTime);
	float omega = 2.0f / st;
	float x     = omega * dt;
	float exp   = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

	CalyxEngine::Vector3 change = current - target;
	CalyxEngine::Vector3 temp   = (vel + change * omega) * dt;
	vel            = (vel - temp * omega) * exp;
	return target + (change + temp) * exp;
}

CalyxEngine::Vector3 Camera3d::GetForward() const {
	return CalyxEngine::Vector3(
	  GetWorldTransform().matrix.world.m[2][0],
	  GetWorldTransform().matrix.world.m[2][1],
	  GetWorldTransform().matrix.world.m[2][2]
   ).Normalize();
}

void Camera3d::AlwaysUpdate(float dt) {
	// 入力等の既存処理
	BaseCamera::AlwaysUpdate(dt);

	// 視錐台更新
	frustum_.ExtractFromMatrix(viewProjectionMatrix_);
	frustum_.Draw();
}

void Camera3d::ShowGui() {
	// 既存のWT GUI
	if (GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::Object)) {
		worldTransform_.ShowImGui("world");
		GuiCmd::EndSection();
	}

}

void Camera3d::GetShadowFrustumCorners(CalyxEngine::Vector3 outCorners[8], float shadowFar) const {
	const float cameraFar = farZ_;
	float ratio = 1.0f;
	if (cameraFar > 1e-6f) ratio = (std::min)(shadowFar / cameraFar, 1.0f);

	frustum_.CalculateCorners(outCorners, ratio);
}


bool Camera3d::IsVisible(const AABB& aabb) const { return frustum_.IsAABBInside(aabb.min_,aabb.max_); }

REGISTER_SCENE_OBJECT(Camera3d)