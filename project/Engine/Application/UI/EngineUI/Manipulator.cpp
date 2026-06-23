#include "Manipulator.h"
/* ========================================================================
/* include space
/* ===================================================================== */
#include "Engine/Application/Settings/EngineSettings.h"
#include "Engine/Application/System/Environment.h"
#include "Engine/Assets/Manager/AssetManager.h"

#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Editor/SceneObjectEditor.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/MathUtil.h>
#include <Engine/Foundation/Utility/Func/CxUtils.h>
#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/System/Command/Manager/CommandManager.h>

#include <algorithm>
#include <cmath>

namespace CalyxEngine {

	namespace {
		void SyncEffectTransformTarget(WorldTransform* target) {
			if(!target) return;

			auto* context = SceneContext::Current();
			if(!context || !context->GetObjectLibrary()) return;

			for(const auto& object : context->GetObjectLibrary()->GetAllObjectsShared()) {
				if(!object || &object->GetWorldTransform() != target) continue;

				if(auto fx = std::dynamic_pointer_cast<FxObject>(object)) {
					fx->SyncChildrenFromWorldTransform();
				} else if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(object)) {
					ps->SyncEmitterFromWorldTransform();
				}
				return;
			}
		}
	}

	Manipulator::Manipulator() {
		iconTranslate_.texture = reinterpret_cast<ImTextureID>(AssetManager::GetInstance()->GetTextureManager()->LoadTexture("UI/Tool/translate.dds").ptr);
		iconRotate_.texture    = reinterpret_cast<ImTextureID>(AssetManager::GetInstance()->GetTextureManager()->LoadTexture("UI/Tool/rotate.dds").ptr);
		iconScale_.texture     = reinterpret_cast<ImTextureID>(AssetManager::GetInstance()->GetTextureManager()->LoadTexture("UI/Tool/scale.dds").ptr);
		iconUniversal_.texture = reinterpret_cast<ImTextureID>(AssetManager::GetInstance()->GetTextureManager()->LoadTexture("UI/Tool/universal.dds").ptr);
		iconWorld_.texture     = reinterpret_cast<ImTextureID>(AssetManager::GetInstance()->GetTextureManager()->LoadTexture("UI/Tool/world.dds").ptr);
		iconDrawGrid_.texture  = reinterpret_cast<ImTextureID>(AssetManager::GetInstance()->GetTextureManager()->LoadTexture("UI/Tool/grid.dds").ptr);
		snapIcon_.texture      = reinterpret_cast<ImTextureID>(AssetManager::GetInstance()->GetTextureManager()->LoadTexture("UI/Tool/snap.png").ptr);
		SetOverlayAlign(OverlayAlign::TopLeft);
		SetOverlayOffset(overlayOffset_); // Viewport右上から左下に少しずらす

		ApplySettings(EngineSettings::GetInstance()->GetData().manipulator);
	}

	void Manipulator::SetTarget(WorldTransform* target) {
		if(target_ != target) { target_ = target; }
		targets_.clear();
		if(target) { targets_.push_back(target); }
	}

	void Manipulator::SetTargets(const std::vector<WorldTransform*>& targets) {
		targets_.clear();
		for(auto* target : targets) {
			if(!target) continue;
			if(std::find(targets_.begin(),targets_.end(),target) != targets_.end()) continue;
			targets_.push_back(target);
		}
		target_ = targets_.empty() ? nullptr : targets_.back();
		if(targets_.size() <= 1) { return; }
		RefreshPivot();
	}

	void Manipulator::SetCamera(BaseCamera* camera) { camera_ = camera; }

	void Manipulator::SetViewRect(const ImVec2& origin,const ImVec2& size) {
		viewOrigin_ = origin;
		viewSize_   = size;
	}

	void Manipulator::Update() {}

	void Manipulator::ApplySettings(const ManipulatorSettings& settings) { settings_ = settings; }

	ManipulatorSettings Manipulator::GetSettings() const { return settings_; }

	void Manipulator::RefreshPivot() {
		if(targets_.empty() || wasUsing) return;

		Vector3 center = Vector3::Zero();
		int     count  = 0;
		for(auto* target : targets_) {
			if(!target) continue;
			center += target->GetWorldPosition();
			++count;
		}
		if(count == 0) return;

		center /= static_cast<float>(count);
		pivotTarget_.Initialize();
		pivotTarget_.scale       = {1.0f,1.0f,1.0f};
		pivotTarget_.rotation    = Quaternion::MakeIdentity();
		pivotTarget_.translation = center;
		pivotTarget_.Update();
	}

	void Manipulator::ApplyWorldMatrix(WorldTransform* target,const Matrix4x4& worldEdited) {
		if(!target) return;

		float worldCol[16];
		RowToColumnArray(worldEdited,worldCol);
		float pW[3],rW[3],sW[3];
		ImGuizmo::DecomposeMatrixToComponents(worldCol,pW,rW,sW);

		Matrix4x4 rotMat = worldEdited;
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
		rotMat.m[3][0]            = rotMat.m[3][1] = rotMat.m[3][2] = 0.0f;
		rotMat.m[3][3]            = 1.0f;
		const Quaternion worldRot = Quaternion::FromMatrix(rotMat);

		if(target->parent) {
			Vector3 pScl = {
				Vector3(target->parent->matrix.world.m[0][0],target->parent->matrix.world.m[0][1],target->parent->matrix.world.m[0][2]).Length(),
				Vector3(target->parent->matrix.world.m[1][0],target->parent->matrix.world.m[1][1],target->parent->matrix.world.m[1][2]).Length(),
				Vector3(target->parent->matrix.world.m[2][0],target->parent->matrix.world.m[2][1],target->parent->matrix.world.m[2][2]).Length()};

			Matrix4x4 pRotMat = target->parent->matrix.world;
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
			pRotMat.m[3][3] = 1.0f;

			const Quaternion pRotQ = Quaternion::FromMatrix(pRotMat);
			const Vector3    pPos  = {
				target->parent->matrix.world.m[3][0],
				target->parent->matrix.world.m[3][1],
				target->parent->matrix.world.m[3][2]};

			const Vector3    effPScl = target->inheritScale ? pScl : Vector3{1,1,1};
			const Quaternion effPRot = target->inheritRotate ? pRotQ : Quaternion::MakeIdentity();
			const Vector3    effPPos = target->inheritTranslate ? pPos : Vector3{0,0,0};

			Vector3 localTrans = Quaternion::RotateVector(
				Vector3{pW[0] - effPPos.x,pW[1] - effPPos.y,pW[2] - effPPos.z},
				Quaternion::Inverse(effPRot));
			if(std::abs(effPScl.x) > 0.0001f) localTrans.x /= effPScl.x;
			if(std::abs(effPScl.y) > 0.0001f) localTrans.y /= effPScl.y;
			if(std::abs(effPScl.z) > 0.0001f) localTrans.z /= effPScl.z;

			Vector3 localScale = {sW[0],sW[1],sW[2]};
			if(std::abs(effPScl.x) > 0.0001f) localScale.x /= effPScl.x;
			if(std::abs(effPScl.y) > 0.0001f) localScale.y /= effPScl.y;
			if(std::abs(effPScl.z) > 0.0001f) localScale.z /= effPScl.z;

			target->translation = localTrans;
			target->scale       = localScale;
			target->rotation    = Quaternion::Multiply(worldRot,Quaternion::Inverse(effPRot));
		} else {
			target->translation = {pW[0],pW[1],pW[2]};
			target->scale       = {sW[0],sW[1],sW[2]};
			target->rotation    = worldRot;
		}

		target->rotationSource = RotationSource::Quaternion;
		target->Update();
		SyncEffectTransformTarget(target);
	}

	void Manipulator::Manipulate() {
		if(is2DMode_) return;
		if(camera_ != SceneContext::Current()->GetCameraMgr()->GetDebug()) { camera_ = SceneContext::Current()->GetCameraMgr()->GetDebug(); }

		if(targets_.empty() || !camera_) return;

		const bool groupMode = targets_.size() > 1;
		if(groupMode) {
			RefreshPivot();
			target_ = &pivotTarget_;
		} else { target_ = targets_.front(); }

		if(!target_) return;

		float view[16],proj[16],world[16];

		// カメラビュー、プロジェクションを転置して列優先配列に変換
		Matrix4x4::Transpose(camera_->GetViewMatrix()).CopyToArray(view);
		Matrix4x4::Transpose(camera_->GetProjectionMatrix()).CopyToArray(proj);

		// 操作対象のワールド行列を転置して列優先配列に変換
		Matrix4x4::Transpose(target_->matrix.world).CopyToArray(world);

		// 操作対象のワールド行列でManipulateを呼ぶ
		float  snapValues[3] = {0.0f,0.0f,0.0f};
		float* snapPtr       = nullptr;

		if(settings_.useSnap) {
			if(operation_ & ImGuizmo::TRANSLATE) {
				snapValues[0] = settings_.snapTranslate[0];
				snapValues[1] = settings_.snapTranslate[1];
				snapValues[2] = settings_.snapTranslate[2];
				snapPtr       = snapValues;
			} else if(operation_ & ImGuizmo::ROTATE) {
				snapValues[0] = settings_.snapRotate;
				snapPtr       = snapValues;
			} else if(operation_ & ImGuizmo::SCALE) {
				snapValues[0] = settings_.snapScale[0];
				snapValues[1] = settings_.snapScale[1];
				snapValues[2] = settings_.snapScale[2];
				snapPtr       = snapValues;
			}
		}
		ImGuizmo::Manipulate(view,proj,operation_,mode_,world,nullptr,snapPtr,nullptr,nullptr);

		bool usingNow = ImGuizmo::IsUsing();

		if(usingNow && !wasUsing) {
			skipGizmoCommandThisDrag_  = false;
			const bool duplicateByDrag =
				(ImGui::GetIO().KeyCtrl && (operation_ & ImGuizmo::TRANSLATE) && onCtrlTranslateDuplicate_);
			if(duplicateByDrag) {
				std::vector<WorldTransform*> duplicatedTargets = onCtrlTranslateDuplicate_();
				if(!duplicatedTargets.empty()) {
					skipGizmoCommandThisDrag_ = true;
					SetTargets(duplicatedTargets);
					const bool duplicatedGroupMode = targets_.size() > 1;
					if(duplicatedGroupMode) {
						RefreshPivot();
						target_ = &pivotTarget_;
					} else { target_ = targets_.front(); }
				}
			}

			groupStartWorlds_.clear();
			groupStartWorlds_.reserve(targets_.size());
			for(auto* target : targets_) { groupStartWorlds_.push_back(target ? target->matrix.world : Matrix4x4::MakeIdentity()); }
			groupStartPivot_ = pivotTarget_.matrix.world;
			if(!skipGizmoCommandThisDrag_) { scopedCmd = std::make_unique<ScopedGizmoCommand>(targets_,operation_); }
		}

		if(usingNow) {
			Matrix4x4 worldEdited = ColumnArrayToRow(world);

			if(groupMode) {
				if(groupStartWorlds_.size() != targets_.size()) {
					groupStartWorlds_.clear();
					groupStartWorlds_.reserve(targets_.size());
					for(auto* target : targets_) { groupStartWorlds_.push_back(target ? target->matrix.world : Matrix4x4::MakeIdentity()); }
					groupStartPivot_ = pivotTarget_.matrix.world;
				}

				const Matrix4x4 delta = Matrix4x4::Inverse(groupStartPivot_) * worldEdited;
				for(size_t i = 0; i < targets_.size(); ++i) {
					auto* target = targets_[i];
					if(!target || i >= groupStartWorlds_.size()) continue;
					ApplyWorldMatrix(target,groupStartWorlds_[i] * delta);
				}

				pivotTarget_.matrix.world = worldEdited;
				pivotTarget_.translation  = Matrix4x4::Translation(worldEdited);
			} else {
				float wE[16];
				RowToColumnArray(worldEdited,wE);
				float pW[3],rW[3],sW[3];
				ImGuizmo::DecomposeMatrixToComponents(wE,pW,rW,sW);

				WorldTransform* worldTarget = dynamic_cast<WorldTransform*>(target_);
				if(worldTarget && worldTarget->parent) {
					Matrix4x4 effP     = worldTarget->GetEffectiveParentMatrix();
					Matrix4x4 localMat = worldEdited * Matrix4x4::Inverse(effP);

					float localCol[16];
					RowToColumnArray(localMat,localCol);
					float pL[3],rL[3],sL[3];
					ImGuizmo::DecomposeMatrixToComponents(localCol,pL,rL,sL);

					// --- Rigid Inverse Reconstruction (Updateの合成ロジックの逆計算) ---

					// 親の情報を取得 (Updateと同じ方法で抽出)
					Vector3 pScl = {
						Vector3(worldTarget->parent->matrix.world.m[0][0],worldTarget->parent->matrix.world.m[0][1],worldTarget->parent->matrix.world.m[0][2]).Length(),
						Vector3(worldTarget->parent->matrix.world.m[1][0],worldTarget->parent->matrix.world.m[1][1],worldTarget->parent->matrix.world.m[1][2]).Length(),
						Vector3(worldTarget->parent->matrix.world.m[2][0],worldTarget->parent->matrix.world.m[2][1],worldTarget->parent->matrix.world.m[2][2]).Length()};

					Matrix4x4 pRotMat = worldTarget->parent->matrix.world;
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
					pRotMat.m[3][0]  = pRotMat.m[3][1] = pRotMat.m[3][2] = 0.0f;
					pRotMat.m[3][3]  = 1.0f;
					Quaternion pRotQ = Quaternion::FromMatrix(pRotMat);

					Vector3 pPos = {worldTarget->parent->matrix.world.m[3][0],worldTarget->parent->matrix.world.m[3][1],worldTarget->parent->matrix.world.m[3][2]};

					Vector3    effPScl = worldTarget->inheritScale ? pScl : Vector3{1,1,1};
					Quaternion effPRot = worldTarget->inheritRotate ? pRotQ : Quaternion::MakeIdentity();
					Vector3    effPPos = worldTarget->inheritTranslate ? pPos : Vector3{0,0,0};

					// 操作モードに応じて変更箇所を絞る
					if(operation_ & ImGuizmo::TRANSLATE) {
						// worldPos = effPPos + effPRot * (effPScl * localPos)
						// localPos = (effPRot.Inv * (worldPos - effPPos)) / effPScl
						Vector3 diff       = {pW[0] - effPPos.x,pW[1] - effPPos.y,pW[2] - effPPos.z};
						Vector3 localTrans = Quaternion::RotateVector(diff,Quaternion::Inverse(effPRot));
						if(std::abs(effPScl.x) > 0.0001f) localTrans.x /= effPScl.x;
						if(std::abs(effPScl.y) > 0.0001f) localTrans.y /= effPScl.y;
						if(std::abs(effPScl.z) > 0.0001f) localTrans.z /= effPScl.z;
						target_->translation = localTrans;
					}
					if(operation_ & ImGuizmo::SCALE) {
						// worldScl = localScl * effPScl
						Vector3 localScl = {sW[0],sW[1],sW[2]};
						if(std::abs(effPScl.x) > 0.0001f) localScl.x /= effPScl.x;
						if(std::abs(effPScl.y) > 0.0001f) localScl.y /= effPScl.y;
						if(std::abs(effPScl.z) > 0.0001f) localScl.z /= effPScl.z;
						target_->scale = localScl;
					}
					if(operation_ & ImGuizmo::ROTATE) {
						// worldRot = localRot * effPRot
						// localRot = worldRot * effPRot.Inv
						Matrix4x4 wRotMat = worldEdited;
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
						wRotMat.m[3][3] = 1.0f;

						Quaternion worldRot     = Quaternion::FromMatrix(wRotMat);
						target_->rotation       = Quaternion::Multiply(worldRot,Quaternion::Inverse(effPRot));
						target_->rotationSource = RotationSource::Quaternion;
					}
				} else {
					// 親がない、またはWorldTransformでない場合は world 直接
					// 操作モードに応じて変更箇所を絞る
					if(operation_ & ImGuizmo::TRANSLATE) { target_->translation = {pW[0],pW[1],pW[2]}; }
					if(operation_ & ImGuizmo::SCALE) { target_->scale = {sW[0],sW[1],sW[2]}; }
					if(operation_ & ImGuizmo::ROTATE) {
						Matrix4x4 rotMat = worldEdited;
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

						target_->rotation       = Quaternion::FromMatrix(rotMat);
						target_->rotationSource = RotationSource::Quaternion;
					}
				}
			}
		}

		// Undoコマンド管理
		if(usingNow && !groupMode && target_) {
			target_->Update();
			SyncEffectTransformTarget(target_);
		}

		if(!usingNow && wasUsing && scopedCmd) {
			scopedCmd->CaptureAfter();
			if(!scopedCmd->IsTrivial())
				CommandManager::GetInstance()->Execute(std::move(scopedCmd));
			else
				scopedCmd.reset();
		}
		if(!usingNow && wasUsing) {
			groupStartWorlds_.clear();
			RefreshPivot();
			skipGizmoCommandThisDrag_ = false;
		}
		wasUsing = usingNow;
	}


	void Manipulator::RenderToolButtons(const ImVec2& basePos, bool allowUniversal, float& nextY) {

		ImVec2 iconSize = iconTranslate_.size;
		float  spacing  = 10.0f;

		struct ButtonInfo {
			ImGuizmo::OPERATION      op;
			const char*              tooltip;
			const Manipulator::Icon& icon;
		};

		ButtonInfo buttons[] = {
			{ImGuizmo::TRANSLATE,"Translate",iconTranslate_},
			{ImGuizmo::ROTATE,"Rotate",iconRotate_},
			{ImGuizmo::SCALE,"Scale",iconScale_},
			{ImGuizmo::UNIVERSAL,"Universal",iconUniversal_}};

		for(int i = 0; i < IM_ARRAYSIZE(buttons); ++i) {
			if(!allowUniversal && buttons[i].op == ImGuizmo::UNIVERSAL) continue;
			ImVec2 btnPos = ImVec2(basePos.x,basePos.y + i * (iconSize.y + spacing));
			ImGui::SetCursorScreenPos(btnPos);

			bool isSelected = (operation_ == buttons[i].op);
			if(isSelected)
				ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(1.00f,0.45f,0.25f,1.00f));

			if(ImGui::ImageButton(buttons[i].icon.texture,iconSize))
				operation_ = buttons[i].op;

			if(isSelected)
				ImGui::PopStyleColor();

			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("%s",buttons[i].tooltip);
		}
		nextY = basePos.y + (allowUniversal ? IM_ARRAYSIZE(buttons) : IM_ARRAYSIZE(buttons) - 1) * (iconSize.y + spacing);
	}

	void Manipulator::Render2DOverlay(const ImVec2& basePos) {
		if(targets_.empty()) return;
		target_ = targets_.front();
		if(!target_) return;

		float nextY = basePos.y;
		RenderToolButtons(basePos, false, nextY);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		target_->Update();
		const float scaleX = viewSize_.x > 0.0f ? viewSize_.x / static_cast<float>(kGameWidth) : 1.0f;
		const float scaleY = viewSize_.y > 0.0f ? viewSize_.y / static_cast<float>(kGameHeight) : 1.0f;
		const ImVec2 pivot(
			viewOrigin_.x + target_->translation.x * scaleX,
			viewOrigin_.y + target_->translation.y * scaleY);
		const ImVec2 size(
			(std::max)(1.0f, target_->scale.x * scaleX),
			(std::max)(1.0f, target_->scale.y * scaleY));
		const ImVec2 rectMin(pivot.x - size.x * anchor2D_.x, pivot.y - size.y * anchor2D_.y);
		const ImVec2 rectMax(rectMin.x + size.x, rectMin.y + size.y);

		drawList->AddRect(rectMin, rectMax, IM_COL32(255, 174, 80, 255), 0.0f, 0, 2.0f);
		drawList->AddCircleFilled(pivot, 4.0f, IM_COL32(255, 120, 64, 255));

		ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Y;
		if(operation_ & ImGuizmo::ROTATE) {
			op = ImGuizmo::ROTATE_Z;
		} else if(operation_ & ImGuizmo::SCALE) {
			op = ImGuizmo::SCALE_X | ImGuizmo::SCALE_Y;
		}

		float view[16], proj[16], world[16];
		Matrix4x4::Transpose(Matrix4x4::MakeIdentity()).CopyToArray(view);
		Matrix4x4::Transpose(MakeOrthographicMatrixLH(
			0.0f,
			static_cast<float>(kGameWidth),
			static_cast<float>(kGameHeight),
			0.0f,
			-1.0f,
			1.0f)).CopyToArray(proj);
		Matrix4x4::Transpose(target_->matrix.world).CopyToArray(world);

		float snapValues[3] = {0.0f, 0.0f, 0.0f};
		float* snapPtr = nullptr;
		if(settings_.useSnap) {
			if(op & (ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Y)) {
				snapValues[0] = settings_.snapTranslate[0];
				snapValues[1] = settings_.snapTranslate[1];
				snapValues[2] = 0.0f;
				snapPtr = snapValues;
			} else if(op & ImGuizmo::ROTATE_Z) {
				snapValues[0] = settings_.snapRotate;
				snapPtr = snapValues;
			} else if(op & (ImGuizmo::SCALE_X | ImGuizmo::SCALE_Y)) {
				snapValues[0] = settings_.snapScale[0];
				snapValues[1] = settings_.snapScale[1];
				snapValues[2] = 1.0f;
				snapPtr = snapValues;
			}
		}

		ImGuizmo::Manipulate(view, proj, op, ImGuizmo::LOCAL, world, nullptr, snapPtr, nullptr, nullptr);
		if(ImGuizmo::IsUsing()) {
			Matrix4x4 edited = ColumnArrayToRow(world);
			float editedColumn[16];
			RowToColumnArray(edited, editedColumn);
			float pos[3], rot[3], scl[3];
			ImGuizmo::DecomposeMatrixToComponents(editedColumn, pos, rot, scl);

			if(op & (ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Y)) {
				target_->translation.x = pos[0];
				target_->translation.y = pos[1];
			}
			if(op & (ImGuizmo::SCALE_X | ImGuizmo::SCALE_Y)) {
				target_->scale.x = (std::max)(1.0f, scl[0]);
				target_->scale.y = (std::max)(1.0f, scl[1]);
			}
			if(op & ImGuizmo::ROTATE_Z) {
				target_->eulerRotation.x = 0.0f;
				target_->eulerRotation.y = 0.0f;
				target_->eulerRotation.z = ToRadians(rot[2]);
				target_->rotationSource = RotationSource::Euler;
			}
			target_->Update();
		}
	}

	void Manipulator::RenderOverlay(const ImVec2& basePos) {
		if(is2DMode_) {
			if(activeViewportType_ == ViewportType::VIEWPORT_MAIN) {
				Render2DOverlay(basePos);
			}
			return;
		}
		if(activeViewportType_ != ViewportType::VIEWPORT_DEBUG) return;

		Manipulate();

		float nextY = basePos.y;
		RenderToolButtons(basePos, true, nextY);

		ImVec2 iconSize = iconTranslate_.size;
		float  spacing  = 10.0f;

		// ワールド/ローカル切り替えボタン
		{
			ImVec2 btnPos = ImVec2(basePos.x,nextY);
			ImGui::SetCursorScreenPos(btnPos);

			bool isWorld = (mode_ == ImGuizmo::WORLD);
			if(isWorld)
				ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(1.00f,0.45f,0.25f,1.00f));

			if(ImGui::ImageButton(iconWorld_.texture,iconSize))
				mode_ = isWorld ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

			if(isWorld)
				ImGui::PopStyleColor();

			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("%s Mode",isWorld ? "World" : "Local");
		}

		{
			EditorSettings editorSettings = EngineSettings::GetInstance()->GetData().editor;
			spacing += 15.0f;
			ImVec2 btnPos = ImVec2(basePos.x,nextY + iconSize.y + spacing);
			ImGui::SetCursorScreenPos(btnPos);

			bool pushStyle = false;
			if(editorSettings.showEditorGrid) {
				ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(1.00f,0.45f,0.25f,1.00f));
				pushStyle = true;
			}

			if(ImGui::ImageButton(iconDrawGrid_.texture,iconSize)) {
				editorSettings.showEditorGrid = !editorSettings.showEditorGrid;
				EngineSettings::GetInstance()->SetEditorSettings(editorSettings);
			}

			if(pushStyle) {
				ImGui::PopStyleColor(); // Push したときだけ Pop する
			}
		}

		{
			ImVec2    snapPos   = ImVec2(basePos.x,nextY + (iconSize.y + spacing) * 2.0f);
			ImGui::SetCursorScreenPos(snapPos);
			ImGui::PushID("ManipulatorSnap");
			bool settingsChanged = false;

			bool pushStyle = false;
			if(settings_.useSnap) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.00f, 0.45f, 0.25f, 1.00f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.00f, 0.55f, 0.35f, 1.00f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.00f, 0.40f, 0.20f, 1.00f));
				pushStyle = true;
			}
			if(ImGui::ImageButton(snapIcon_.texture, snapIcon_.size)) {
				settingsChanged = true;
				settings_.useSnap = !settings_.useSnap;
			}
			if(pushStyle) {
				ImGui::PopStyleColor(3);
			}

			if(settings_.useSnap) {
				ImGui::SetNextItemWidth(120.0f);
				if(operation_ & ImGuizmo::TRANSLATE) {
					settingsChanged |= ImGui::InputFloat3("Move Step",settings_.snapTranslate);
				} else if(operation_ & ImGuizmo::ROTATE) {
					settingsChanged |= ImGui::InputFloat("Angle Step",&settings_.snapRotate);
				} else if(operation_ & ImGuizmo::SCALE) {
					settingsChanged |= ImGui::InputFloat3("Scale Step",settings_.snapScale);
				}
			}
			if(settingsChanged) { EngineSettings::GetInstance()->SetManipulatorSettings(settings_); }
			ImGui::PopID();
		}
	}

	void Manipulator::RenderToolbar() {}

	void Manipulator::RowToColumnArray(const Matrix4x4& m,float out[16]) {
		// 回転スケール 3×3 を転置（row→column変換）
		out[0]  = m.m[0][0];
		out[1]  = m.m[0][1];
		out[2]  = m.m[0][2];
		out[3]  = 0.0f;
		out[4]  = m.m[1][0];
		out[5]  = m.m[1][1];
		out[6]  = m.m[1][2];
		out[7]  = 0.0f;
		out[8]  = m.m[2][0];
		out[9]  = m.m[2][1];
		out[10] = m.m[2][2];
		out[11] = 0.0f;

		out[12] = m.m[3][0];
		out[13] = m.m[3][1];
		out[14] = m.m[3][2];
		out[15] = 1.0f;
	}

	Matrix4x4 Manipulator::ColumnArrayToRow(const float in_[16]) {
		Matrix4x4 m;
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


} // namespace CalyxEngine
