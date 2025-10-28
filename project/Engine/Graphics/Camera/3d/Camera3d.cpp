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

// c++
#include <cmath>

namespace {

// 列ベース: 左手系行列 M の回転部分から Right/Up/Forward を正規化して取り出す（スケール除去）
static void ExtractBasisNoScale(const Matrix4x4& M, Vector3& R, Vector3& U, Vector3& F){
	R = Vector3(M.m[0][0], M.m[0][1], M.m[0][2]).Normalize(); // +X
	U = Vector3(M.m[1][0], M.m[1][1], M.m[1][2]).Normalize(); // +Y
	F = Vector3(M.m[2][0], M.m[2][1], M.m[2][2]).Normalize(); // +Z (LH: forward)
}

// 位置のアフィン変換（点）
static inline Vector3 TransformPoint(const Matrix4x4& M, const Vector3& p){
	return {
		M.m[0][0]*p.x + M.m[1][0]*p.y + M.m[2][0]*p.z + M.m[3][0],
		M.m[0][1]*p.x + M.m[1][1]*p.y + M.m[2][1]*p.z + M.m[3][1],
		M.m[0][2]*p.x + M.m[1][2]*p.y + M.m[2][2]*p.z + M.m[3][2],
	};
}

// 方向のアフィン変換（ベクトル：平行移動なし）
static inline Vector3 TransformDirection(const Matrix4x4& M, const Vector3& v){
	return {
		M.m[0][0]*v.x + M.m[1][0]*v.y + M.m[2][0]*v.z,
		M.m[0][1]*v.x + M.m[1][1]*v.y + M.m[2][1]*v.z,
		M.m[0][2]*v.x + M.m[1][2]*v.y + M.m[2][2]*v.z,
	};
}

// 回転行列だけを正規直交化（親にスケールが乗っていても回転を復元）
static Matrix4x4 OrthonormalizeRotation(const Matrix4x4& M){
	Vector3 R,U,F; ExtractBasisNoScale(M, R,U,F);
	// 再直交化（Gram-Schmidt 簡易）
	R = R.Normalize();
	U = (U - R * Vector3::Dot(R,U)).Normalize();
	F = Vector3::Cross(R, U).Normalize(); // LHなら R×U=F でOK

	Matrix4x4 Rm{};
	Rm.m[0][0]=R.x; Rm.m[0][1]=R.y; Rm.m[0][2]=R.z; Rm.m[0][3]=0;
	Rm.m[1][0]=U.x; Rm.m[1][1]=U.y; Rm.m[1][2]=U.z; Rm.m[1][3]=0;
	Rm.m[2][0]=F.x; Rm.m[2][1]=F.y; Rm.m[2][2]=F.z; Rm.m[2][3]=0;
	Rm.m[3][0]=0;   Rm.m[3][1]=0;   Rm.m[3][2]=0;   Rm.m[3][3]=1;
	return Rm;
}

} // namespace

//--------------------------------- ctor ---------------------------------
Camera3d::Camera3d() : BaseCamera() {
	BaseCamera::SetName("MainCamera");
	worldTransform_.translation = {0.0f,2.0f,-10.0f};
}

Camera3d::Camera3d(const std::string& name) { SceneObject::SetName(name,ObjectType::Camera); }

void Camera3d::Initialize() { worldTransform_.translation = {0.0f,2.0f,-10.0f}; }

float Camera3d::ExpLerpAlpha(float dt,float tau) {
	if(tau <= 1e-6f) return 1.0f;
	return 1.0f - std::exp(-dt / tau);
}

Vector3 Camera3d::SmoothDampVec(const Vector3& current,const Vector3& target,
								Vector3&       vel,float              smoothTime,float dt) {
	// Unity の SmoothDamp 近似
	float st    = (std::max)(0.0001f,smoothTime);
	float omega = 2.0f / st;
	float x     = omega * dt;
	float exp   = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

	Vector3 change = current - target;
	Vector3 temp   = (vel + change * omega) * dt;
	vel            = (vel - temp * omega) * exp;
	return target + (change + temp) * exp;
}

void Camera3d::UpdateFollow(float dt) {
	 if (!follow_.target) return;

    //  ターゲットのワールド行列から基底を抽出（スケール無視）
    const Matrix4x4& Tw = follow_.target->matrix.world; // あなたの WorldTransform の world 行列
    Vector3 Tr { Tw.m[3][0], Tw.m[3][1], Tw.m[3][2] }; // target world pos
    Vector3 R,U,F; ExtractBasisNoScale(Tw, R,U,F);      // world right/up/forward

    //  望ましい「カメラのワールド位置」
    Vector3 desiredWorldPos =
        Tr
        - F * follow_.distanceBack
        + U * follow_.heightOffset
        + R * follow_.sideOffset;

    //  位置スムージングはワールドで
    //    （posVel もワールド速度として保持）
    Vector3 curWorldPos = TransformPoint(worldTransform_.matrix.world, {0,0,0}); // 現在のワールド位置
    curWorldPos = SmoothDampVec(curWorldPos, desiredWorldPos, follow_.posVel, follow_.posSmoothTime, dt);

    //  望ましい「カメラのワールド回転」
    //    ターゲットのワールド回転行列（スケール除去→直交化）からクォータニオンへ
    Matrix4x4 targetRotM = OrthonormalizeRotation(Tw);
    Quaternion targetWorldQ = Quaternion::FromMatrix(targetRotM);

    //    俯角はワールドの Right 軸（R）回り
    Quaternion extraPitch = Quaternion::MakeRotateAxisQuaternion(
        R, Cx::Math::ToRadians(follow_.extraPitchDeg));
    Quaternion desiredWorldQ = extraPitch * targetWorldQ;

    //    現在のワールド回転を取得
    Matrix4x4 CwRotM = OrthonormalizeRotation(worldTransform_.matrix.world);
    Quaternion curWorldQ = Quaternion::FromMatrix(CwRotM);

    //    ワールド回転を指数補間
    float a = ExpLerpAlpha(dt, follow_.rotTimeConstant);
    Quaternion newWorldQ = Quaternion::Slerp(curWorldQ, desiredWorldQ, a);

    // 親のローカルに戻してセット
    if (worldTransform_.parent){
        Matrix4x4 Pw = worldTransform_.parent->matrix.world;
        Matrix4x4 PwInv = Matrix4x4::Inverse(Pw);

        // ローカル位置 = 親^-1 * ワールド位置
        Vector3 localPos = TransformPoint(PwInv, curWorldPos);

        // ローカル回転 = 親回転^-1 * ワールド回転
        Matrix4x4 parentRotM = OrthonormalizeRotation(Pw);
        Matrix4x4 parentRotInv = Matrix4x4::Transpose(parentRotM); // 直交なので転置=逆
        Matrix4x4 newWorldRotM = Quaternion::ToMatrix(newWorldQ);
        Matrix4x4 localRotM = parentRotInv * newWorldRotM;
        Quaternion localQ = Quaternion::FromMatrix(localRotM);

        worldTransform_.translation = localPos;
        worldTransform_.rotation    = localQ;
    } else {
        // 親なしならそのまま
        worldTransform_.translation = curWorldPos;
        worldTransform_.rotation    = newWorldQ;
    }
}

void Camera3d::AlwaysUpdate(float dt) {
	// 入力等の既存処理
	BaseCamera::AlwaysUpdate(dt);

	// 追従
	if(follow_.enabled && follow_.target) { UpdateFollow(dt); }

	// 視錐台更新
	frustum_.ExtractFromMatrix(viewProjectionMatrix_);
	frustum_.Draw();
}

void Camera3d::ShowGui() {
	// 既存のWT GUI
	worldTransform_.ShowImGui();

	if(ImGui::CollapsingHeader("Follow Target##Camera3d",ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Enabled",&follow_.enabled);
		ImGui::DragFloat("Distance Back",&follow_.distanceBack,0.05f,0.0f,1000.0f);
		ImGui::DragFloat("Height Offset",&follow_.heightOffset,0.05f,-100.0f,100.0f);
		ImGui::DragFloat("Side Offset",&follow_.sideOffset,0.05f,-100.0f,100.0f);
		ImGui::DragFloat3("LookAt Offset",&follow_.lookAtOffset.x,0.05f);

		ImGui::SeparatorText("Smoothing");
		ImGui::DragFloat("Pos Smooth Time",&follow_.posSmoothTime,0.005f,0.01f,1.0f);
		ImGui::DragFloat("Rot Time Constant",&follow_.rotTimeConstant,0.005f,0.01f,1.0f);
		ImGui::DragFloat("Extra Pitch (deg)",&follow_.extraPitchDeg,0.1f,-89.0f,89.0f);

		if(follow_.target) ImGui::Text("Target: set");
		else ImGui::TextDisabled("Target: (null)");
	}
}

bool Camera3d::IsVisible(const AABB& aabb) const { return frustum_.IsAABBInside(aabb.min_,aabb.max_); }

REGISTER_SCENE_OBJECT(Camera3d)