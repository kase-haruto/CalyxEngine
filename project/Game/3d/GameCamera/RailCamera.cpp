#include "RailCamera.h"

#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Foundation/Utility/Func/MathFunc.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Application/Input/Input.h>

// C++
#include <cmath>
#include <algorithm>

REGISTER_SCENE_OBJECT(RailCamera)

RailCamera::RailCamera() {}
RailCamera::RailCamera(const std::string& name) {
	SceneObject::SetName(name, ObjectType::Camera);
}

void RailCamera::Initialize() {
	worldTransform_.Initialize();
	worldTransform_.translation = { 0.0f, 10.0f, 0.0f };
	BaseCamera::SetName("RailCamera");

	// 既定値
	speed_ = 45.0f;
	lookAhead_ = 2.0f;
	tiltAngle_ = 0.3f;
	tiltLerpSpeed_ = 10.0f;
	targetTilt_ = 0.0f;
	zTiltOffset_ = 0.0f;
	traveled_ = 0.0f;

	// デフォルトのスプライン読み込み
	const std::string defaultPath = "Resources/Assets/Spline/Rail.json";
	if (!LoadSplineFromJson(defaultPath)) {
		spline_ = SplineData{};
		arc_.clear();
		totalLength_ = 0.0f;
	}
}

void RailCamera::SetSpline(const SplineData& s) {
	spline_ = s;
	RebuildArcTable();    // 等速テーブルを再構築
	traveled_ = 0.0f;     // 先頭へ
}

bool RailCamera::LoadSplineFromJson(const std::string& path) {
	SplineData tmp;
	if (!SplineJson::Load(path, tmp)) { // JSON から読み込み
		return false;
	}
	SetSpline(tmp);
	return true;
}

void RailCamera::ClearSpline() {
	spline_ = SplineData{};
	arc_.clear();
	totalLength_ = 0.0f;
	traveled_ = 0.0f;
}

// ----------------------------------------------------------------------------
// 補助: スプライン評価
// ----------------------------------------------------------------------------
Vector3 RailCamera::Eval(float t) const {
	return spline_.Evaluate(t); // Catmull–Rom 補間（closed対応）:contentReference[oaicite:3]{index=3}
}

// ----------------------------------------------------------------------------
// 補助: 弧長テーブル作成（等速化用）
// ----------------------------------------------------------------------------
void RailCamera::RebuildArcTable() {
	arc_.clear();
	totalLength_ = 0.0f;

	// 最低限の安全チェック
	if (spline_.points.size() == 0) {
		return;
	}
	if (spline_.points.size() == 1) {
		arc_.push_back({ 0.0f, 0.0f, spline_.points[0].pos });
		totalLength_ = 0.0f;
		return;
	}

	// サンプリング密度（必要なら増やす）
	const int kSamples = 1000;
	arc_.reserve(kSamples + 1);

	Vector3 prev = Eval(0.0f);
	arc_.push_back({ 0.0f, 0.0f, prev });

	float acc = 0.0f;
	for (int i = 1; i <= kSamples; ++i) {
		float t = static_cast<float>(i) / static_cast<float>(kSamples);
		Vector3 p = Eval(t);
		acc += (p - prev).Length();
		arc_.push_back({ t, acc, p });
		prev = p;
	}
	totalLength_ = acc;
}

// ----------------------------------------------------------------------------
// 補助: 弧長→t（二分探索）
// ----------------------------------------------------------------------------
float RailCamera::DistanceToT(float s) const {
	if (arc_.empty()) return 0.0f;
	if (s <= 0.0f) return 0.0f;
	if (s >= totalLength_) return 1.0f;

	int lo = 0, hi = static_cast<int>(arc_.size()) - 1;
	while (lo < hi) {
		int mid = (lo + hi) / 2;
		if (arc_[mid].dist < s) lo = mid + 1;
		else hi = mid;
	}
	// 線形補間
	const ArcSample& a = arc_[(std::max)(0, lo - 1)];
	const ArcSample& b = arc_[lo];
	float span = (std::max)(1e-6f, b.dist - a.dist);
	float lt = (s - a.dist) / span;
	return a.t + (b.t - a.t) * lt;
}

// ----------------------------------------------------------------------------
// 進行方向（look-ahead）とロール
// ----------------------------------------------------------------------------
void RailCamera::UpdateOrientationFromPath(float dt) {
	// 現在位置
	float tNow = (totalLength_ > 0.0f) ? DistanceToT(traveled_) : 0.0f;
	Vector3 eye = Eval(tNow);

	// 先読み点（進行方向）
	float sAhead = traveled_ + (std::max)(lookAhead_, 0.01f);
	float tAhead = (totalLength_ > 0.0f) ? DistanceToT(
		spline_.closed ? std::fmod(sAhead, totalLength_) : std::min(sAhead, totalLength_)
	) : 0.0f;
	Vector3 target = Eval(tAhead);

	// 向き
	Vector3 dir = (target - eye);
	float len = dir.Length();
	if (len > 1e-4f) dir = dir / len;
	else dir = { 0,0,1 };

	// オイラーへの変換（Y-up前提）
	float horizontalDist = std::sqrt(dir.x * dir.x + dir.z * dir.z);
	worldTransform_.eulerRotation.x = std::atan2(-dir.y, (std::max)(1e-6f, horizontalDist));
	worldTransform_.eulerRotation.y = std::atan2(dir.x, dir.z);

	// ロール（曲率由来の簡易バンク）
	// 2点先読みで左右曲がり具合を推定 → [-1,1] に正規化して tiltAngle_ をスケール
	float sAhead2 = traveled_ + lookAhead_ * 2.0f;
	float tAhead2 = (totalLength_ > 0.0f) ? DistanceToT(
		spline_.closed ? std::fmod(sAhead2, totalLength_) : std::min(sAhead2, totalLength_)
	) : 0.0f;
	Vector3 p0 = eye;
	Vector3 p1 = target;
	Vector3 p2 = Eval(tAhead2);

	Vector3 v1 = (p1 - p0); float l1 = v1.Length();
	Vector3 v2 = (p2 - p1); float l2 = v2.Length();
	if (l1 > 1e-4f) v1 /= l1;
	if (l2 > 1e-4f) v2 /= l2;

	// 横方向の曲がり＝法線成分を Y-up 前提でスカラー化（右旋回で負、左旋回で正にして自然な傾き）
	float turn = v1.x * v2.z - v1.z * v2.x; // 2Dクロス（XZ平面）
	turn = std::clamp(turn, -1.0f, 1.0f);
	targetTilt_ = -turn * tiltAngle_; // 右に曲がると右に傾く（-）

	// ロール補間
	zTiltOffset_ = std::lerp(zTiltOffset_, targetTilt_, tiltLerpSpeed_ * dt);
	worldTransform_.eulerRotation.z = zTiltOffset_;

	// 位置更新
	worldTransform_.translation = eye;
}

void RailCamera::Update(float dt) {
	// 入力で速度調整したい場合はここで（例：↑で加速、↓で減速）
	// auto& in = *Input::GetInstance();
	// if (in.Pressed(KeyCode::Up))   speed_ += 5.0f * dt;
	// if (in.Pressed(KeyCode::Down)) speed_ -= 5.0f * dt;

	// 走行弧長を更新（等速）
	traveled_ += (std::max)(0.0f, speed_) * dt;

	if (spline_.closed) {
		// ループ
		if (totalLength_ > 0.0f) {
			traveled_ = std::fmod(traveled_, totalLength_);
			if (traveled_ < 0.0f) traveled_ += totalLength_;
		} else {
			traveled_ = 0.0f;
		}
	} else {
		// 非ループは末端で止める
		traveled_ = std::clamp(traveled_, 0.0f, totalLength_);
	}

	UpdateOrientationFromPath(dt);
}

void RailCamera::ShowGui() {
	worldTransform_.ShowImGui();
	if (ImGui::CollapsingHeader("RailCamera")) {
		ImGui::DragFloat("Speed (units/s)", &speed_, 0.1f, 0.0f, 1000.0f);
		ImGui::DragFloat("LookAhead", &lookAhead_, 0.01f, 0.0f, 100.0f);
		ImGui::DragFloat("TiltAngle (rad)", &tiltAngle_, 0.01f, 0.0f, 1.57f);
		ImGui::DragFloat("TiltLerp", &tiltLerpSpeed_, 0.1f, 0.0f, 50.0f);

		ImGui::Text("Spline: %zu pts, closed=%s, length=%.2f",
					spline_.points.size(), spline_.closed ? "true" : "false", totalLength_);

				// デバッグ：位置手動調整
		float tNow = (totalLength_ > 1e-6f) ? DistanceToT(traveled_) : 0.0f;
		if (ImGui::SliderFloat("t (debug)", &tNow, 0.0f, 1.0f)) {
			traveled_ = totalLength_ * tNow;
		}

		// 軌道再構築（外部編集後に押す）
		if (ImGui::Button("Rebuild Arc Table")) {
			RebuildArcTable();
		}
	}
}

void RailCamera::AlwaysUpdate(float dt) {
	BaseCamera::AlwaysUpdate(dt);
}

Vector3 RailCamera::GetPosition() {
	return worldTransform_.GetWorldPosition();
}
