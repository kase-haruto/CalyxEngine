#include "Manipulator.h"
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/Input/Input.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Editor/SceneObjectEditor.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/System/Command/EditorCommand/GuizmoCommand/ScopedGizmoCommand.h>
#include <Engine/System/Command/Manager/CommandManager.h>

void Manipulator::SetTarget(WorldTransform* target){
	target_ = target;
}

void Manipulator::SetCamera(BaseCamera* camera){
	camera_ = camera;
}

void Manipulator::SetViewRect(const ImVec2& origin, const ImVec2& size){
	viewOrigin_ = origin;
	viewSize_ = size;
}

Manipulator::Manipulator() {
	//iconの初期化
	iconTranslate_.texture = reinterpret_cast<ImTextureID>(TextureManager::GetInstance()->LoadTexture("UI/Tool/translate.png").ptr);
	iconRotate_.texture = reinterpret_cast<ImTextureID>(TextureManager::GetInstance()->LoadTexture("UI/Tool/rotate.png").ptr);
	iconScale_.texture = reinterpret_cast<ImTextureID>(TextureManager::GetInstance()->LoadTexture("UI/Tool/scale.png").ptr);
	iconUniversal_.texture = reinterpret_cast<ImTextureID>(TextureManager::GetInstance()->LoadTexture("UI/Tool/universal.png").ptr);
	iconWorld_.texture = reinterpret_cast<ImTextureID>(TextureManager::GetInstance()->LoadTexture("UI/Tool/world.png").ptr);
}

void Manipulator::Update(){
	if (!target_ || !camera_) return;

	float view[16], proj[16], world[16];
	Matrix4x4::Transpose(camera_->GetViewMatrix()).CopyToArray(view);
	Matrix4x4::Transpose(camera_->GetProjectionMatrix()).CopyToArray(proj);
	Matrix4x4::Transpose(target_->matrix.world).CopyToArray(world);

	static bool wasUsing = false;
	static std::unique_ptr<ScopedGizmoCommand> scopedCmd;

	ImGuizmo::Manipulate(view, proj, operation_, mode_, world);

	bool usingNow = ImGuizmo::IsUsing();

	if (usingNow){
		Matrix4x4 worldEdited = ColumnArrayToRow(world);

		Matrix4x4 localEdited;

		if (target_->parent){
			// 親の worldMatrix の逆行列を使ってローカル変換を求める
			localEdited = Matrix4x4::Inverse(target_->parent->matrix.world) * worldEdited;
		} else{
			// 親がいなければローカル＝ワールド
			localEdited = worldEdited;
		}

		float decomposed[16];
		RowToColumnArray(localEdited, decomposed);

		float pos[3], rot[3], scl[3];
		ImGuizmo::DecomposeMatrixToComponents(decomposed, pos, rot, scl);

		target_->translation = {pos[0], pos[1], pos[2]};
		target_->scale = {scl[0], scl[1], scl[2]};

		constexpr float DegToRad = 3.14159265f / 180.0f;
		Vector3 euler = {rot[0] * DegToRad, rot[1] * DegToRad, rot[2] * DegToRad};
		target_->rotation = Quaternion::EulerToQuaternion(euler);

		target_->rotationSource = RotationSource::Quaternion;
	}

	if (usingNow && !wasUsing){   // 編集 “開始”
		scopedCmd = std::make_unique<ScopedGizmoCommand>(target_, operation_);
	} else if (!usingNow && wasUsing && scopedCmd){   // 編集 “終了”
		scopedCmd->CaptureAfter();
		if (!scopedCmd->IsTrivial())
			CommandManager::GetInstance()->Execute(std::move(scopedCmd));
		else
			scopedCmd.reset();
	}
	wasUsing = usingNow;
}

void Manipulator::RenderOverlay(){}

void Manipulator::RenderToolbar() {

	ImVec2 iconSize = iconTranslate_.size;

	struct ButtonInfo {
		ImGuizmo::OPERATION op;
		const char* tooltip;
		const Icon& icon;
	};

	ButtonInfo buttons[] = {
		{ ImGuizmo::TRANSLATE, "Translate", iconTranslate_ },
		{ ImGuizmo::ROTATE,    "Rotate",    iconRotate_    },
		{ ImGuizmo::SCALE,     "Scale",     iconScale_     },
		{ ImGuizmo::UNIVERSAL, "Universal", iconUniversal_ }
	};

	for (const auto& btn : buttons) {
		bool isSelected = (operation_ == btn.op);
		if (isSelected) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 1.0f));
		}

		if (ImGui::ImageButton(btn.icon.texture, iconSize)) {
			operation_ = btn.op;
		}
		if (isSelected) {
			ImGui::PopStyleColor();
		}

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", btn.tooltip);
		}
	}

	{
		bool isWorld = (mode_ == ImGuizmo::WORLD);

		if (isWorld) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 1.0f));
		}

		if (ImGui::ImageButton(iconWorld_.texture, iconWorld_.size)) {
			// トグル切り替え
			mode_ = isWorld ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
		}

		if (isWorld) {
			ImGui::PopStyleColor();
		}

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s Mode", isWorld ? "World" : "Local");
		}
	}

}

void Manipulator::RowToColumnArray(const Matrix4x4& m, float out[16]){
	// 回転スケール 3×3 を転置
	out[0] = m.m[0][0]; out[1] = m.m[1][0]; out[2] = m.m[2][0]; out[3] = 0.0f;
	out[4] = m.m[0][1]; out[5] = m.m[1][1]; out[6] = m.m[2][1]; out[7] = 0.0f;
	out[8] = m.m[0][2]; out[9] = m.m[1][2]; out[10] = m.m[2][2]; out[11] = 0.0f;

	out[12] = m.m[3][0];
	out[13] = m.m[3][1];
	out[14] = m.m[3][2];
	out[15] = 1.0f;
}

Matrix4x4 Manipulator::ColumnArrayToRow(const float in_[16]){
	Matrix4x4 m;
	m.m[0][0] = in_[0];  m.m[0][1] = in_[4];  m.m[0][2] = in_[8];  m.m[0][3] = 0.0f;
	m.m[1][0] = in_[1];  m.m[1][1] = in_[5];  m.m[1][2] = in_[9];  m.m[1][3] = 0.0f;
	m.m[2][0] = in_[2];  m.m[2][1] = in_[6];  m.m[2][2] = in_[10]; m.m[2][3] = 0.0f;

	// 平行移動
	m.m[3][0] = in_[12];
	m.m[3][1] = in_[13];
	m.m[3][2] = in_[14];
	m.m[3][3] = 1.0f;
	return m;
}
