#include "Manipulator.h"
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Editor/SceneObjectEditor.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Utility/Func/CxUtils.h>
#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/System/Command/Manager/CommandManager.h>

namespace CalyxEditor {

	Manipulator::Manipulator() {
		iconTranslate_.texture = reinterpret_cast<ImTextureID>(TextureManager::GetInstance()->LoadTexture("UI/Tool/translate.dds").ptr);
		iconRotate_.texture	   = reinterpret_cast<ImTextureID>(TextureManager::GetInstance()->LoadTexture("UI/Tool/rotate.dds").ptr);
		iconScale_.texture	   = reinterpret_cast<ImTextureID>(TextureManager::GetInstance()->LoadTexture("UI/Tool/scale.dds").ptr);
		iconUniversal_.texture = reinterpret_cast<ImTextureID>(TextureManager::GetInstance()->LoadTexture("UI/Tool/universal.dds").ptr);
		iconWorld_.texture	   = reinterpret_cast<ImTextureID>(TextureManager::GetInstance()->LoadTexture("UI/Tool/world.dds").ptr);
		iconDrawGrid_.texture  = reinterpret_cast<ImTextureID>(TextureManager::GetInstance()->LoadTexture("UI/Tool/grid.dds").ptr);
		SetOverlayAlign(OverlayAlign::TopLeft);
		SetOverlayOffset(overlayOffset_); // Viewport右上から左下に少しずらす
	}

	void Manipulator::SetTarget(WorldTransform* target) {
		if(target_ != target) {
			target_ = target;
		}
	}

	void Manipulator::SetCamera(BaseCamera* camera) {
		camera_ = camera;
	}

	void Manipulator::SetViewRect(const ImVec2& origin, const ImVec2& size) {
		viewOrigin_ = origin;
		viewSize_	= size;
	}

	void Manipulator::Update() {
	}

	void Manipulator::Manipulate() {
		if(camera_ != SceneContext::Current()->GetCameraMgr()->GetDebug()) {
			camera_ = SceneContext::Current()->GetCameraMgr()->GetDebug();
		}

		if(!target_ || !camera_) return;

		float view[16], proj[16], world[16], parent[16];

		// カメラビュー、プロジェクションを転置して列優先配列に変換
		CalyxMath::Matrix4x4::Transpose(camera_->GetViewMatrix()).CopyToArray(view);
		CalyxMath::Matrix4x4::Transpose(camera_->GetProjectionMatrix()).CopyToArray(proj);

		// 操作対象のワールド行列を転置して列優先配列に変換
		CalyxMath::Matrix4x4::Transpose(target_->matrix.world).CopyToArray(world);

		// 親がいれば親のワールド行列を渡す。なければ単位行列
		if(target_->parent) {
			CalyxMath::Matrix4x4::Transpose(target_->parent->matrix.world).CopyToArray(parent);
		} else {
			CalyxMath::Matrix4x4 identity;
			identity.MakeIdentity(); // または MakeIdentity()
			CalyxMath::Matrix4x4::Transpose(identity).CopyToArray(parent);
		}

		// 親行列を渡してManipulateを呼ぶ
		ImGuizmo::Manipulate(view, proj, operation_, mode_, world, nullptr, nullptr, nullptr, parent);

		bool usingNow = ImGuizmo::IsUsing();

		if(usingNow) {
			CalyxMath::Matrix4x4 worldEdited = ColumnArrayToRow(world);

			CalyxMath::Matrix4x4 localEdited;
			if(target_->parent) {
				// ここで掛ける順序を逆にしてみる
				localEdited = worldEdited * CalyxMath::Matrix4x4::Inverse(target_->parent->matrix.world);
			} else {
				localEdited = worldEdited;
			}

			// 以降は同じ
			float decomposed[16];
			RowToColumnArray(localEdited, decomposed);

			float pos[3], rotDeg[3], scl[3];
			ImGuizmo::DecomposeMatrixToComponents(decomposed, pos, rotDeg, scl);

			target_->translation = {pos[0], pos[1], pos[2]};
			target_->scale		 = {scl[0], scl[1], scl[2]};

			CalyxMath::Vector3 eulerRad = {
				CalyxMath::ToRadians(rotDeg[0]),
				CalyxMath::ToRadians(rotDeg[1]),
				CalyxMath::ToRadians(rotDeg[2])};
			target_->rotation		= CalyxMath::Quaternion::EulerToQuaternion(eulerRad);
			target_->rotationSource = RotationSource::Quaternion;
		}

		// Undoコマンド管理はそのまま
		if(usingNow && !wasUsing) {
			scopedCmd = std::make_unique<ScopedGizmoCommand>(target_, operation_);
		} else if(!usingNow && wasUsing && scopedCmd) {
			scopedCmd->CaptureAfter();
			if(!scopedCmd->IsTrivial())
				CommandManager::GetInstance()->Execute(std::move(scopedCmd));
			else
				scopedCmd.reset();
		}
		wasUsing = usingNow;
	}

	void Manipulator::RenderOverlay(const ImVec2& basePos) {
		Manipulate();

		ImVec2 iconSize = iconTranslate_.size;
		float  spacing	= 10.0f;

		struct ButtonInfo {
			ImGuizmo::OPERATION		 op;
			const char*				 tooltip;
			const Manipulator::Icon& icon;
		};

		ButtonInfo buttons[] = {
			{ImGuizmo::TRANSLATE, "Translate", iconTranslate_},
			{ImGuizmo::ROTATE, "Rotate", iconRotate_},
			{ImGuizmo::SCALE, "Scale", iconScale_},
			{ImGuizmo::UNIVERSAL, "Universal", iconUniversal_}};

		for(int i = 0; i < IM_ARRAYSIZE(buttons); ++i) {
			ImVec2 btnPos = ImVec2(basePos.x, basePos.y + i * (iconSize.y + spacing));
			ImGui::SetCursorScreenPos(btnPos);

			bool isSelected = (operation_ == buttons[i].op);
			if(isSelected)
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.00f, 0.45f, 0.25f, 1.00f));

			if(ImGui::ImageButton(buttons[i].icon.texture, iconSize))
				operation_ = buttons[i].op;

			if(isSelected)
				ImGui::PopStyleColor();

			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", buttons[i].tooltip);
		}

		// ワールド/ローカル切り替えボタン
		{
			int	   i	  = IM_ARRAYSIZE(buttons);
			ImVec2 btnPos = ImVec2(basePos.x, basePos.y + i * (iconSize.y + spacing));
			ImGui::SetCursorScreenPos(btnPos);

			bool isWorld = (mode_ == ImGuizmo::WORLD);
			if(isWorld)
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.00f, 0.45f, 0.25f, 1.00f));

			if(ImGui::ImageButton(iconWorld_.texture, iconSize))
				mode_ = isWorld ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

			if(isWorld)
				ImGui::PopStyleColor();

			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("%s Mode", isWorld ? "World" : "Local");
		}

		{
			static bool showGrid = false;
			int			i		 = IM_ARRAYSIZE(buttons);
			spacing += 15.0f;
			ImVec2 btnPos = ImVec2(basePos.x, basePos.y + i * (iconSize.y + spacing));
			ImGui::SetCursorScreenPos(btnPos);

			bool pushStyle = false;
			if(showGrid) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.00f, 0.45f, 0.25f, 1.00f));
				pushStyle = true;
			}

			if(ImGui::ImageButton(iconDrawGrid_.texture, iconSize)) {
				showGrid = !showGrid;
			}

			if(pushStyle) {
				ImGui::PopStyleColor(); // Push したときだけ Pop する
			}

			if(showGrid) {
				PrimitiveDrawer::GetInstance()->DrawGrid();
			}
		}
	}

	void Manipulator::RenderToolbar() {
	}

	void Manipulator::RowToColumnArray(const CalyxMath::Matrix4x4& m, float out[16]) {
		// 回転スケール 3×3 を転置（row→column変換）
		out[0]	= m.m[0][0];
		out[1]	= m.m[0][1];
		out[2]	= m.m[0][2];
		out[3]	= 0.0f;
		out[4]	= m.m[1][0];
		out[5]	= m.m[1][1];
		out[6]	= m.m[1][2];
		out[7]	= 0.0f;
		out[8]	= m.m[2][0];
		out[9]	= m.m[2][1];
		out[10] = m.m[2][2];
		out[11] = 0.0f;

		out[12] = m.m[3][0];
		out[13] = m.m[3][1];
		out[14] = m.m[3][2];
		out[15] = 1.0f;
	}

	CalyxMath::Matrix4x4 Manipulator::ColumnArrayToRow(const float in_[16]) {
		CalyxMath::Matrix4x4 m;
		m.m[0][0] = in_[0];
		m.m[0][1] = in_[1];
		m.m[0][2] = in_[2];
		m.m[0][3] = 0.0f;
		m.m[1][0] = in_[4];
		m.m[1][1] = in_[5];
		m.m[1][2] = in_[6];
		m.m[1][3] = 0.0f;
		m.m[2][0] = in_[8];
		m.m[2][1] = in_[9];
		m.m[2][2] = in_[10];
		m.m[2][3] = 0.0f;

		m.m[3][0] = in_[12];
		m.m[3][1] = in_[13];
		m.m[3][2] = in_[14];
		m.m[3][3] = 1.0f;
		return m;
	}

} // namespace CalyxEditor