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

		float view[16], proj[16], world[16];

		// カメラビュー、プロジェクションを転置して列優先配列に変換
		CalyxMath::Matrix4x4::Transpose(camera_->GetViewMatrix()).CopyToArray(view);
		CalyxMath::Matrix4x4::Transpose(camera_->GetProjectionMatrix()).CopyToArray(proj);

		// 操作対象のワールド行列を転置して列優先配列に変換
		CalyxMath::Matrix4x4::Transpose(target_->matrix.world).CopyToArray(world);

		// 操作対象のワールド行列でManipulateを呼ぶ
		// （本来なら第9引数は boundsSnap です。親行列ではないためここでは nullptr にします）
		ImGuizmo::Manipulate(view, proj, operation_, mode_, world, nullptr, nullptr, nullptr, nullptr);

		bool usingNow = ImGuizmo::IsUsing();

		if(usingNow) {
			CalyxMath::Matrix4x4 worldEdited = ColumnArrayToRow(world);

			float wE[16];
			RowToColumnArray(worldEdited, wE);
			float pW[3], rW[3], sW[3];
			ImGuizmo::DecomposeMatrixToComponents(wE, pW, rW, sW);

			WorldTransform* worldTarget = dynamic_cast<WorldTransform*>(target_);
			if(worldTarget && worldTarget->parent) {
				CalyxMath::Matrix4x4 effP	  = worldTarget->GetEffectiveParentMatrix();
				CalyxMath::Matrix4x4 localMat = worldEdited * CalyxMath::Matrix4x4::Inverse(effP);

				float localCol[16];
				RowToColumnArray(localMat, localCol);
				float pL[3], rL[3], sL[3];
				ImGuizmo::DecomposeMatrixToComponents(localCol, pL, rL, sL);

				// --- Rigid Inverse Reconstruction (Updateの合成ロジックの逆計算) ---

				// 親の情報を取得 (Updateと同じ方法で抽出)
				CalyxMath::Vector3 pScl = {
					CalyxMath::Vector3(worldTarget->parent->matrix.world.m[0][0], worldTarget->parent->matrix.world.m[0][1], worldTarget->parent->matrix.world.m[0][2]).Length(),
					CalyxMath::Vector3(worldTarget->parent->matrix.world.m[1][0], worldTarget->parent->matrix.world.m[1][1], worldTarget->parent->matrix.world.m[1][2]).Length(),
					CalyxMath::Vector3(worldTarget->parent->matrix.world.m[2][0], worldTarget->parent->matrix.world.m[2][1], worldTarget->parent->matrix.world.m[2][2]).Length()};

				CalyxMath::Matrix4x4 pRotMat = worldTarget->parent->matrix.world;
				if(pScl.x > 0.0001f) {
					pRotMat.m[0][0] /= pScl.x;
					pRotMat.m[0][1] /= pScl.x;
					pRotMat.m[0][2] /= pScl.x;
				}
				if(pScl.y > 0.0001f) {
					pRotMat.m[1][0] /= pScl.y;
					pRotMat.m[1][1] /= pScl.y;
					pRotMat.m[1][2] /= pScl.y;
				}
				if(pScl.z > 0.0001f) {
					pRotMat.m[2][0] /= pScl.z;
					pRotMat.m[2][1] /= pScl.z;
					pRotMat.m[2][2] /= pScl.z;
				}
				pRotMat.m[3][0] = pRotMat.m[3][1] = pRotMat.m[3][2] = 0.0f;
				pRotMat.m[3][3]										= 1.0f;
				CalyxMath::Quaternion pRotQ							= CalyxMath::Quaternion::FromMatrix(pRotMat);

				CalyxMath::Vector3 pPos = {worldTarget->parent->matrix.world.m[3][0], worldTarget->parent->matrix.world.m[3][1], worldTarget->parent->matrix.world.m[3][2]};

				CalyxMath::Vector3	  effPScl = worldTarget->inheritScale ? pScl : CalyxMath::Vector3{1, 1, 1};
				CalyxMath::Quaternion effPRot = worldTarget->inheritRotate ? pRotQ : CalyxMath::Quaternion::MakeIdentity();
				CalyxMath::Vector3	  effPPos = worldTarget->inheritTranslate ? pPos : CalyxMath::Vector3{0, 0, 0};

				// 操作モードに応じて変更箇所を絞る
				if(operation_ & ImGuizmo::TRANSLATE) {
					// worldPos = effPPos + effPRot * (effPScl * localPos)
					// localPos = (effPRot.Inv * (worldPos - effPPos)) / effPScl
					CalyxMath::Vector3 diff		  = {pW[0] - effPPos.x, pW[1] - effPPos.y, pW[2] - effPPos.z};
					CalyxMath::Vector3 localTrans = CalyxMath::Quaternion::RotateVector(diff, CalyxMath::Quaternion::Inverse(effPRot));
					if(std::abs(effPScl.x) > 0.0001f) localTrans.x /= effPScl.x;
					if(std::abs(effPScl.y) > 0.0001f) localTrans.y /= effPScl.y;
					if(std::abs(effPScl.z) > 0.0001f) localTrans.z /= effPScl.z;
					target_->translation = localTrans;
				}
				if(operation_ & ImGuizmo::SCALE) {
					// worldScl = localScl * effPScl
					CalyxMath::Vector3 localScl = {sW[0], sW[1], sW[2]};
					if(std::abs(effPScl.x) > 0.0001f) localScl.x /= effPScl.x;
					if(std::abs(effPScl.y) > 0.0001f) localScl.y /= effPScl.y;
					if(std::abs(effPScl.z) > 0.0001f) localScl.z /= effPScl.z;
					target_->scale = localScl;
				}
				if(operation_ & ImGuizmo::ROTATE) {
					// worldRot = localRot * effPRot
					// localRot = worldRot * effPRot.Inv
					CalyxMath::Matrix4x4 wRotMat = worldEdited;
					if(std::abs(sW[0]) > 0.0001f) {
						wRotMat.m[0][0] /= sW[0];
						wRotMat.m[0][1] /= sW[0];
						wRotMat.m[0][2] /= sW[0];
					}
					if(std::abs(sW[1]) > 0.0001f) {
						wRotMat.m[1][0] /= sW[1];
						wRotMat.m[1][1] /= sW[1];
						wRotMat.m[1][2] /= sW[1];
					}
					if(std::abs(sW[2]) > 0.0001f) {
						wRotMat.m[2][0] /= sW[2];
						wRotMat.m[2][1] /= sW[2];
						wRotMat.m[2][2] /= sW[2];
					}
					wRotMat.m[3][0] = wRotMat.m[3][1] = wRotMat.m[3][2] = 0.0f;
					wRotMat.m[3][3]										= 1.0f;

					CalyxMath::Quaternion worldRot = CalyxMath::Quaternion::FromMatrix(wRotMat);
					target_->rotation			   = CalyxMath::Quaternion::Multiply(worldRot, CalyxMath::Quaternion::Inverse(effPRot));
					target_->rotationSource		   = RotationSource::Quaternion;
				}

			} else {
				// 親がない、またはWorldTransformでない場合は world 直接
				// 操作モードに応じて変更箇所を絞る
				if(operation_ & ImGuizmo::TRANSLATE) {
					target_->translation = {pW[0], pW[1], pW[2]};
				}
				if(operation_ & ImGuizmo::SCALE) {
					target_->scale = {sW[0], sW[1], sW[2]};
				}
				if(operation_ & ImGuizmo::ROTATE) {
					CalyxMath::Matrix4x4 rotMat = worldEdited;
					if(std::abs(sW[0]) > 0.0001f) {
						rotMat.m[0][0] /= sW[0];
						rotMat.m[0][1] /= sW[0];
						rotMat.m[0][2] /= sW[0];
					}
					if(std::abs(sW[1]) > 0.0001f) {
						rotMat.m[1][0] /= sW[1];
						rotMat.m[1][1] /= sW[1];
						rotMat.m[1][2] /= sW[1];
					}
					if(std::abs(sW[2]) > 0.0001f) {
						rotMat.m[2][0] /= sW[2];
						rotMat.m[2][1] /= sW[2];
						rotMat.m[2][2] /= sW[2];
					}

					target_->rotation		= CalyxMath::Quaternion::FromMatrix(rotMat);
					target_->rotationSource = RotationSource::Quaternion;
				}
			}
		}

		// Undoコマンド管理
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