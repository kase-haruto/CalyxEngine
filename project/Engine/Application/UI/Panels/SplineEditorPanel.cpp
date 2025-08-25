#include "SplineEditorPanel.h"
#include <externals/imgui/imgui.h>
#include <externals/imgui/ImGuiFileDialog.h>

#include <Engine/Objects/3D/Geometory/Spline/SplineJson.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Application/Input/Input.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Physics/Ray/Raycastor.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

#include <cfloat>
#include <algorithm>

// -------------------- ツールバー --------------------
void SplineEditorPanel::DrawToolbar() {
	if (ImGui::Button("New")) {
		data_ = SplineData{}; selectedPoint_ = -1; currentPath_.clear();
	}
	ImGui::SameLine();
	if (ImGui::Button("Open...")) {
		IGFD::FileDialogConfig c; c.path = "Resources/Assets/Splines/";
		ImGuiFileDialog::Instance()->OpenDialog("SplineOpen", "Open Spline", ".json", c);
	}
	ImGui::SameLine();
	if (ImGui::Button("Save")) {
		if (currentPath_.empty()) {
			IGFD::FileDialogConfig c; c.path = "Resources/Assets/Splines/";
			ImGuiFileDialog::Instance()->OpenDialog("SplineSave", "Save Spline", ".json", c);
		} else {
			SplineJson::Save(currentPath_, data_);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Save As...")) {
		IGFD::FileDialogConfig c; c.path = "Resources/Assets/Splines/";
		ImGuiFileDialog::Instance()->OpenDialog("SplineSaveAs", "Save Spline As", ".json", c);
	}

	// FileDialogs
	if (ImGuiFileDialog::Instance()->Display("SplineOpen")) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			currentPath_ = ImGuiFileDialog::Instance()->GetFilePathName();
			SplineJson::Load(currentPath_, data_);
			selectedPoint_ = -1;
		}
		ImGuiFileDialog::Instance()->Close();
	}
	if (ImGuiFileDialog::Instance()->Display("SplineSave")) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			currentPath_ = ImGuiFileDialog::Instance()->GetFilePathName();
			SplineJson::Save(currentPath_, data_);
		}
		ImGuiFileDialog::Instance()->Close();
	}
	if (ImGuiFileDialog::Instance()->Display("SplineSaveAs")) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			currentPath_ = ImGuiFileDialog::Instance()->GetFilePathName();
			SplineJson::Save(currentPath_, data_);
		}
		ImGuiFileDialog::Instance()->Close();
	}

	ImGui::Separator();
	ImGui::Checkbox("Closed", &data_.closed);
	ImGui::SameLine();
	ImGui::Checkbox("Enable Gizmo", &gizmoEnabled_);
	ImGui::Separator();
}

// -------------------- ポイントリスト --------------------
void SplineEditorPanel::DrawPointsList() {
	if (ImGui::Button("Add Point")) {
		data_.InsertPoint((int)data_.points.size(), { 0,0,0 });
		selectedPoint_ = (int)data_.points.size() - 1;
	}
	ImGui::SameLine();
	if (ImGui::Button("Insert Before") && selectedPoint_ >= 0) {
		data_.InsertPoint(selectedPoint_, data_.points[selectedPoint_].pos);
	}
	ImGui::SameLine();
	if (ImGui::Button("Remove") && selectedPoint_ >= 0) {
		data_.RemovePoint(selectedPoint_);
		selectedPoint_ = -1;
	}

	ImGui::Separator();
	ImGui::Text("Points: %d", (int)data_.points.size());
	ImGui::BeginChild("points", ImVec2(0, 160), true);
	for (int i = 0;i < (int)data_.points.size();++i) {
		ImGui::PushID(i);
		bool sel = (selectedPoint_ == i);
		if (ImGui::Selectable(("Point " + std::to_string(i)).c_str(), sel)) {
			selectedPoint_ = i;
		}
		ImGui::PopID();
	}
	ImGui::EndChild();

	if (selectedPoint_ >= 0 && selectedPoint_ < (int)data_.points.size()) {
		auto& p = data_.points[selectedPoint_].pos;
		ImGui::Separator();
		ImGui::Text("Edit Point %d", selectedPoint_);
		ImGui::DragFloat3("Position", &p.x, 0.01f);
	}
}

// -------------------- 2Dプレビュー（XZ） --------------------
void SplineEditorPanel::DrawPreviewXZ() {
	ImVec2 avail = ImGui::GetContentRegionAvail();
	ImVec2 p0 = ImGui::GetCursorScreenPos();
	ImVec2 p1 = { p0.x + avail.x, p0.y + 200.0f };
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImGui::InvisibleButton("preview", ImVec2(avail.x, 200.0f));
	dl->AddRect(p0, p1, IM_COL32(100, 100, 100, 255));

	if (data_.points.size() < 2) return;

	float minx = 1e9f, minz = 1e9f, maxx = -1e9f, maxz = -1e9f;
	auto scan = [&](const Vector3& v) { minx = (std::min)(minx, v.x); minz = (std::min)(minz, v.z); maxx = (std::max)(maxx, v.x); maxz = (std::max)(maxz, v.z); };
	for (auto& pt : data_.points) scan(pt.pos);
	const int steps = (std::max)(2, data_.SegmentCount() * 16);
	for (int i = 0;i <= steps;i++) scan(data_.Evaluate(i / (float)steps));

	float dx = (std::max)(1e-3f, maxx - minx), dz = (std::max)(1e-3f, maxz - minz);
	auto map = [&](const Vector3& v) {
		float u = (v.x - minx) / dx, w = (v.z - minz) / dz;
		return ImVec2(p0.x + u * (p1.x - p0.x - 6) + 3, p1.y - (w * (p1.y - p0.y - 6) + 3));
	};

	ImVec2 prev = map(data_.Evaluate(0.0f));
	for (int i = 1;i <= steps;i++) {
		ImVec2 cur = map(data_.Evaluate(i / (float)steps));
		dl->AddLine(prev, cur, IM_COL32(0, 200, 255, 255), 2.0f);
		prev = cur;
	}
	for (int i = 0;i < (int)data_.points.size();++i) {
		ImU32 col = (i == selectedPoint_) ? IM_COL32(255, 180, 0, 255) : IM_COL32(255, 255, 255, 220);
		dl->AddCircleFilled(map(data_.points[i].pos), 3.5f, col);
	}
}

// -------------------- レイ生成＆ギズモ --------------------
Ray SplineEditorPanel::MakeMouseRay() const {
	auto* cam = CameraManager::GetDebug();

	ImVec2 m = ImGui::GetMousePos();
	Vector2 mouseLocal{ m.x - vpPos_.x, m.y - vpPos_.y };

	// ビューポート外なら前方へ（当たらないレイ）
	if (mouseLocal.x < 0 || mouseLocal.y < 0 || mouseLocal.x > vpSize_.x || mouseLocal.y > vpSize_.y) {
		return Ray{ cam->GetWorldTransform().GetWorldPosition(), cam->GetTranslate().Forward().Normalize()};
	}
	return Raycastor::ConvertMouseToRay(mouseLocal, cam->GetViewMatrix(), cam->GetProjectionMatrix(), vpSize_);
}

// 交差判定（点のAABB）
struct LocalAABB { Vector3 min, max; };
static bool IntersectRayAABB_Local(const Ray& ray, const LocalAABB& aabb, float& tOut) {
	float tmin = 0.0f, tmax = tOut;
	for (int i = 0;i < 3;++i) {
		float dir = ray.direction[i];
		float invD = 1.0f / (std::abs(dir) < 1e-8f ? (dir >= 0 ? 1e-8f : -1e-8f) : dir);
		float t0 = (aabb.min[i] - ray.origin[i]) * invD;
		float t1 = (aabb.max[i] - ray.origin[i]) * invD;
		if (invD < 0.0f) std::swap(t0, t1);
		tmin = ((std::max))(tmin, t0);
		tmax = ((std::min))(tmax, t1);
		if (tmax < tmin) return false;
	}
	tOut = tmin;
	return true;
}

int SplineEditorPanel::PickPointByRayAABB(const Ray& ray, float halfSize, float& outT) const {
	int best = -1; float bestT = FLT_MAX;
	for (int i = 0;i < (int)data_.points.size(); ++i) {
		const Vector3& p = data_.points[i].pos;
		LocalAABB box{ p - Vector3{halfSize,halfSize,halfSize}, p + Vector3{halfSize,halfSize,halfSize} };
		float t = 1e6f;
		if (IntersectRayAABB_Local(ray, box, t) && t < bestT) {
			bestT = t; best = i;
		}
	}
	outT = bestT;
	return best;
}

bool SplineEditorPanel::IntersectPlane(const Ray& ray, const Vector3& n, float d, Vector3& out) const {
	float denom = Vector3::Dot(n, ray.direction);
	if (std::abs(denom) < 1e-6f) return false;
	float t = -(Vector3::Dot(n, ray.origin) + d) / denom;
	if (t < 0) return false;
	out = ray.origin + ray.direction * t;
	return true;
}

// -------------------- ギズモ更新＋3Dプレビュー描画 --------------------
void SplineEditorPanel::HandleGizmoUpdateAndDraw3D() {
	if (data_.points.empty()) return;
	auto* drawer = PrimitiveDrawer::GetInstance();

	// AABBスケールの基準（全体境界の1%）
	Vector3 minP{ FLT_MAX, FLT_MAX, FLT_MAX }, maxP{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
	auto expand = [&](const Vector3& v) { minP = Vector3::Min(minP, v); maxP = Vector3::Max(maxP, v); };
	for (auto& pt : data_.points) expand(pt.pos);
	const int steps = ((std::max))(2, data_.SegmentCount() * 16);
	for (int i = 0;i <= steps;++i) expand(data_.Evaluate(i / (float)steps));

	Vector3 diag = maxP - minP;
	float aabbHalf = ((std::max))({ diag.x, diag.y, diag.z }) * 0.01f;
	if (aabbHalf <= 0.0f) aabbHalf = 0.05f;

	// 入力 → レイ → ピック/ドラッグ
	if (gizmoEnabled_) {
		Input* in = Input::GetInstance();
		Ray ray = MakeMouseRay();

		if (in->TriggerMouseButton(MouseButton::Left)) {
			float t;
			int idx = PickPointByRayAABB(ray, aabbHalf, t);
			if (idx >= 0) {
				SetSelectedIndex(idx);
				Vector3 p = data_.points[idx].pos;
				dragPlaneN_ = { 0,1,0 };                       // 水平平面（必要なら切替UIを追加）
				dragPlaneD_ = -Vector3::Dot(dragPlaneN_, p);  // N·X + D = 0
				dragging_ = true;
			} else {
				dragging_ = false;
			}
		} else if (in->ReleaseMouseButton(MouseButton::Left)) {
			dragging_ = false;
		}

		if (dragging_ && GetSelectedIndex() >= 0) {
			Vector3 hit{};
			if (IntersectPlane(ray, dragPlaneN_, dragPlaneD_, hit)) {
				data_.points[GetSelectedIndex()].pos = hit;
			}
		}
	}

	// 3Dプレビュー
	drawer->DrawAABB(minP, maxP, Vector4(0.0f, 0.8f, 0.9f, 1.0f)); // 全体AABB

	Vector3 prev = data_.Evaluate(0.0f);
	for (int i = 1;i <= steps;i++) {
		Vector3 cur = data_.Evaluate(i / (float)steps);
		drawer->DrawLine3d(prev, cur, Vector4(0.0f, 0.8f, 0.9f, 1.0f)); // 曲線ライン
		prev = cur;
	}

	int sel = GetSelectedIndex();
	for (int i = 0;i < (int)data_.points.size();++i) {
		const Vector3& p = data_.points[i].pos;
		Vector3 pmin = p - Vector3{ aabbHalf,aabbHalf,aabbHalf };
		Vector3 pmax = p + Vector3{ aabbHalf,aabbHalf,aabbHalf };
		Vector4 col = (i == sel) ? Vector4(1.0f, 0.85f, 0.0f, 1.0f) : Vector4(1.0f, 1.0f, 1.0f, 0.9f);
		drawer->DrawAABB(pmin, pmax, col); // 点のAABB
	}
}

// -------------------- Render 本体 --------------------
void SplineEditorPanel::Render() {
	if (!IsShow()) return;

	bool open = true;
	ImGui::Begin(panelName_.c_str(), &open);

	DrawToolbar();
	DrawPointsList();
	ImGui::Separator();
	DrawPreviewXZ();

	HandleGizmoUpdateAndDraw3D();

	ImGui::End();
	if (!open) SetShow(false);
}
