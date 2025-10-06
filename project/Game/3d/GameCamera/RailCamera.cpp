#include "RailCamera.h"

#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Application/Input/Input.h>

// C++
#include <cmath>
#include <algorithm>

// externals
#include <externals/imgui/imgui.h>

REGISTER_SCENE_OBJECT(RailCamera)

/////////////////////////////////////////////////////////////////////////////////////////
//  コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
RailCamera::RailCamera() {}
RailCamera::RailCamera(const std::string& name) {
	SceneObject::SetName(name, ObjectType::Camera);
}

/////////////////////////////////////////////////////////////////////////////////////////
//  初期化
/////////////////////////////////////////////////////////////////////////////////////////
void RailCamera::Initialize() {
	worldTransform_.Initialize();
	worldTransform_.translation = { 0.0f, 10.0f, 0.0f };
	BaseCamera::SetName("RailCamera");

	// ==== 初期値設定 ====
	speed_ = 35.0f;   // 速度（距離単位 / 秒）
	lookAhead_ = 2.0f;    // 進行方向の先読み距離
	tiltAngle_ = 0.3f;    // バンク角の最大値（ラジアン）
	tiltLerpSpeed_ = 10.0f;   // バンク補間速度
	targetTilt_ = 0.0f;    // 目標バンク角
	zTiltOffset_ = 0.0f;    // 現在のバンク角
	traveled_ = 0.0f;    // 移動距離の累積

	// ==== デフォルトのスプライン読み込み ====
	const std::string defaultPath = "Resources/Assets/Spline/Rail.json";
	if (!LoadSplineFromJson(defaultPath)) {
		spline_ = SplineData{};
		spline_.closed = false;
		spline_.BuildArcTable(arcSamplesPerSeg_);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//  スプライン設定
/////////////////////////////////////////////////////////////////////////////////////////
void RailCamera::SetSpline(const SplineData& s) {
	spline_ = s;
	spline_.BuildArcTable(arcSamplesPerSeg_); // 等速移動のために弧長LUTを再構築
	traveled_ = 0.0f; // 先頭へリセット
}

/////////////////////////////////////////////////////////////////////////////////////////
//  スプライン読み込み
/////////////////////////////////////////////////////////////////////////////////////////
bool RailCamera::LoadSplineFromJson(const std::string& path) {
	SplineData tmp;
	if (!SplineJson::Load(path, tmp)) { // JSON から読み込み
		return false;
	}
	SetSpline(tmp);
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
//  スプライン削除
/////////////////////////////////////////////////////////////////////////////////////////
void RailCamera::ClearSpline() {
	spline_ = SplineData{};
	spline_.BuildArcTable(arcSamplesPerSeg_);
	traveled_ = 0.0f;
}

/////////////////////////////////////////////////////////////////////////////////////////
//  進行方向（look-ahead）とロール計算
/////////////////////////////////////////////////////////////////////////////////////////
void RailCamera::UpdateOrientationFromPath(float dt) {
	// ==== 現在位置 ====
	const float totalLen = spline_.TotalLength();
	float tNow = (totalLen > 0.0f) ? spline_.DistanceToT(traveled_) : 0.0f;
	Vector3 eye = spline_.Evaluate(tNow);

	// ==== 先読み点（進行方向計算用） ====
	float sAhead = traveled_ + (std::max)(lookAhead_, 0.01f);
	if (spline_.closed && totalLen > 0.0f) {
		sAhead = std::fmod(sAhead, totalLen);
		if (sAhead < 0.0f) sAhead += totalLen;
	} else {
		sAhead = std::clamp(sAhead, 0.0f, totalLen);
	}
	float tAhead = (totalLen > 0.0f) ? spline_.DistanceToT(sAhead) : 0.0f;
	Vector3 target = spline_.Evaluate(tAhead);

	// ==== 向きベクトル ====
	Vector3 dir = target - eye;
	float len = dir.Length();
	if (len > 1e-4f) dir /= len;
	else dir = { 0,0,1 };

	// ==== オイラー角への変換（Y-up 前提） ====
	float horizontalDist = std::sqrt(dir.x * dir.x + dir.z * dir.z);
	worldTransform_.eulerRotation.x = std::atan2(-dir.y, (std::max)(1e-6f, horizontalDist));
	worldTransform_.eulerRotation.y = std::atan2(dir.x, dir.z);

	// ==== ロール（曲率による簡易バンク） ====
	float sAhead2 = traveled_ + lookAhead_ * 2.0f;
	if (spline_.closed && totalLen > 0.0f) {
		sAhead2 = std::fmod(sAhead2, totalLen);
		if (sAhead2 < 0.0f) sAhead2 += totalLen;
	} else {
		sAhead2 = std::clamp(sAhead2, 0.0f, totalLen);
	}
	float tAhead2 = (totalLen > 0.0f) ? spline_.DistanceToT(sAhead2) : 0.0f;
	Vector3 p0 = eye;
	Vector3 p1 = target;
	Vector3 p2 = spline_.Evaluate(tAhead2);

	Vector3 v1 = (p1 - p0);
	Vector3 v2 = (p2 - p1);
	if (v1.LengthSquared() > 1e-8f) v1.Normalize();
	if (v2.LengthSquared() > 1e-8f) v2.Normalize();

	// XZ 平面のクロスで旋回方向を検出
	float turn = v1.x * v2.z - v1.z * v2.x;
	turn = std::clamp(turn, -1.0f, 1.0f);
	targetTilt_ = -turn * tiltAngle_; // 右旋回で右に傾く

	// ==== ロール補間 ====
	zTiltOffset_ = std::lerp(zTiltOffset_, targetTilt_, tiltLerpSpeed_ * dt);
	worldTransform_.eulerRotation.z = zTiltOffset_;

	// ==== カメラ位置更新 ====
	worldTransform_.translation = eye;
}

/////////////////////////////////////////////////////////////////////////////////////////
//  更新処理（カメラの前進制御）
/////////////////////////////////////////////////////////////////////////////////////////
void RailCamera::Update(float dt) {
	// ==== 走行距離を更新（等速） ====
	traveled_ += (std::max)(0.0f, speed_) * dt;
	const float totalLen = spline_.TotalLength();

	if (spline_.closed) {
		// ==== ループ再生 ====
		if (totalLen > 0.0f) {
			traveled_ = std::fmod(traveled_, totalLen);
			if (traveled_ < 0.0f) traveled_ += totalLen;
		} else {
			traveled_ = 0.0f;
		}
	} else {
		// ==== 非ループ時は 9割で停止 ====
		const float maxS = totalLen * stopRatio_;
		traveled_ = std::clamp(traveled_, 0.0f, (std::max)(0.0f, maxS));
	}

	UpdateOrientationFromPath(dt);
}

/////////////////////////////////////////////////////////////////////////////////////////
//  デバッグ表示
/////////////////////////////////////////////////////////////////////////////////////////
void RailCamera::ShowGui() {
#if defined(_DEBUG) || defined(DEVELOP)
	worldTransform_.ShowImGui();
	if (ImGui::CollapsingHeader("RailCamera")) {
		ImGui::DragFloat("Speed (units/s)", &speed_, 0.1f, 0.0f, 1000.0f);
		ImGui::DragFloat("LookAhead", &lookAhead_, 0.01f, 0.0f, 100.0f);
		ImGui::DragFloat("TiltAngle (rad)", &tiltAngle_, 0.01f, 0.0f, 1.57f);
		ImGui::DragFloat("TiltLerp", &tiltLerpSpeed_, 0.1f, 0.0f, 50.0f);
		ImGui::DragInt("Arc Samples/Seg", &arcSamplesPerSeg_, 1, 4, 128);
		bool closed = spline_.closed;
		if (ImGui::Checkbox("Closed", &closed)) {
			spline_.closed = closed;
			spline_.BuildArcTable(arcSamplesPerSeg_);
		}
		ImGui::DragFloat("StopRatio", &stopRatio_, 0.01f, 0.0f, 1.0f);

		ImGui::Text("Spline: %zu pts, closed=%s, length=%.2f",
					spline_.points.size(), spline_.closed ? "true" : "false", spline_.TotalLength());

				// ==== デバッグ：位置を手動で調整 ====
		float tNow = (spline_.TotalLength() > 1e-6f) ? spline_.DistanceToT(traveled_) : 0.0f;
		float tMax = spline_.closed ? 1.0f : stopRatio_;
		if (ImGui::SliderFloat("t (debug)", &tNow, 0.0f, tMax)) {
			traveled_ = spline_.TotalLength() * tNow;
		}

		// ==== 弧長テーブル再構築 ====
		if (ImGui::Button("Rebuild Arc Table")) {
			spline_.BuildArcTable(arcSamplesPerSeg_);
		}
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////
//  毎フレーム更新（基本カメラ処理）
/////////////////////////////////////////////////////////////////////////////////////////
void RailCamera::AlwaysUpdate(float dt) {
	BaseCamera::AlwaysUpdate(dt);
}

/////////////////////////////////////////////////////////////////////////////////////////
//  カメラ位置の取得
/////////////////////////////////////////////////////////////////////////////////////////
Vector3 RailCamera::GetPosition() {
	return worldTransform_.GetWorldPosition();
}

/////////////////////////////////////////////////////////////////////////////////////////
//  パラメータ設定
/////////////////////////////////////////////////////////////////////////////////////////
void RailCamera::SetTilt(float angleRad, float lerp) {
	tiltAngle_ = angleRad;
	tiltLerpSpeed_ = lerp;
}

void RailCamera::SetClosed(bool closed) {
	spline_.closed = closed;
	spline_.BuildArcTable(arcSamplesPerSeg_);
}

void RailCamera::SetStopRatio(float r) {
	stopRatio_ = std::clamp(r, 0.0f, 1.0f);
}

/////////////////////////////////////////////////////////////////////////////////////////
//  T値の取得（0〜1）
/////////////////////////////////////////////////////////////////////////////////////////
float RailCamera::GetT() const {
	const float totalLen = spline_.TotalLength();
	if (totalLen <= 1e-6f) return 0.0f;

	float s = traveled_;
	if (spline_.closed && totalLen > 0.0f) {
		float mod = std::fmod(s, totalLen);
		if (mod < 0.0f) mod += totalLen;
		s = mod;
	}
	s = std::clamp(s, 0.0f, totalLen); // 非ループ時は stopRatio_ でクランプ済み
	return spline_.DistanceToT(s);
}

/////////////////////////////////////////////////////////////////////////////////////////
//  進捗率の取得（0〜1）
/////////////////////////////////////////////////////////////////////////////////////////
float RailCamera::GetProgress() const {
	const float totalLen = spline_.TotalLength();
	if (totalLen <= 1e-6f) return 0.0f;

	float s = traveled_;
	if (spline_.closed && totalLen > 0.0f) {
		float mod = std::fmod(s, totalLen);
		if (mod < 0.0f) mod += totalLen;
		s = mod;
	}
	s = std::clamp(s, 0.0f, totalLen);
	return (totalLen > 0.0f) ? (s / totalLen) : 0.0f;
}
