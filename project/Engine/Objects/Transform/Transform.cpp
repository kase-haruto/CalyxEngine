#include "Transform.h"
/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/Foundation/Utility/Func/CxUtils.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>

// data
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

// lib
#include "Engine/Application/System/Environment.h"

#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <externals/imgui/imgui.h>

using namespace CalyxMath;

/////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
void EulerTransform::ShowImGui(const std::string& label) {
	ImGui::SeparatorText(label.c_str());
	std::string scaleLabel		 = label + "_scale";
	std::string rotationLabel	 = label + "_rotation";
	std::string translationLabel = label + "_translate";
	GuiCmd::DragFloat3(scaleLabel.c_str(), scale);
	GuiCmd::DragFloat3(rotationLabel.c_str(), rotate);
	GuiCmd::DragFloat3(translationLabel.c_str(), translate);
}

/////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void BaseTransform::Initialize() {
	// デフォルト値
	scale.Initialize(1.0f);
	rotation.Initialize();

	// バッファの作成
	DxConstantBuffer::Initialize(GraphicsGroup::GetInstance()->GetDevice());

	Update();
}

/////////////////////////////////////////////////////////////////////////////////////////
//	imgui
/////////////////////////////////////////////////////////////////////////////////////////
void BaseTransform::ShowImGui(const std::string& label) {
	std::string nodeLabel = label + "##node";
	// 小さめの折りたたみ見出しとして TreeNodeEx を使用
	if(ImGui::TreeNodeEx(nodeLabel.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
		if(GuiCmd::ColoredDragFloat3("Scale", scale, 0.01f)) {}

		if(GuiCmd::ColoredDragFloat3("Rotation", eulerRotation, 0.1f, -360.0f, 360.0f, "%.1f", "°")) {
			rotationSource = RotationSource::Euler;
		}

		if(GuiCmd::ColoredDragFloat3("Translation", translation, 0.01f)) {}

		ImGui::TreePop();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//	WorldTransform: imgui
/////////////////////////////////////////////////////////////////////////////////////////
void WorldTransform::ShowImGui(const std::string& label) {
	std::string nodeLabel = label + "##node";
	// 小さめの折りたたみ見出しとして TreeNodeEx を使用
	if(ImGui::TreeNodeEx(nodeLabel.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
		bool bInherit = parent != nullptr;

		if(bInherit) {
			if(ImGui::Checkbox("##iScale", &inheritScale)) {
				// 継承フラグが切り替わった瞬間に、現在のワールド状態を維持するようにローカル値を逆算(Fixup)
				CalyxMath::Vector3 wPos = GetWorldPosition();
				CalyxMath::Vector3 wScl = {
					CalyxMath::Vector3(matrix.world.m[0][0], matrix.world.m[0][1], matrix.world.m[0][2]).Length(),
					CalyxMath::Vector3(matrix.world.m[1][0], matrix.world.m[1][1], matrix.world.m[1][2]).Length(),
					CalyxMath::Vector3(matrix.world.m[2][0], matrix.world.m[2][1], matrix.world.m[2][2]).Length()};
				CalyxMath::Matrix4x4 wRotMat = matrix.world;
				if(wScl.x > 0.0001f) {
					wRotMat.m[0][0] /= wScl.x;
					wRotMat.m[0][1] /= wScl.x;
					wRotMat.m[0][2] /= wScl.x;
				}
				if(wScl.y > 0.0001f) {
					wRotMat.m[1][0] /= wScl.y;
					wRotMat.m[1][1] /= wScl.y;
					wRotMat.m[1][2] /= wScl.y;
				}
				if(wScl.z > 0.0001f) {
					wRotMat.m[2][0] /= wScl.z;
					wRotMat.m[2][1] /= wScl.z;
					wRotMat.m[2][2] /= wScl.z;
				}
				wRotMat.m[3][0] = wRotMat.m[3][1] = wRotMat.m[3][2] = 0.0f;
				wRotMat.m[3][3]										= 1.0f;
				CalyxMath::Quaternion wRotQ							= CalyxMath::Quaternion::FromMatrix(wRotMat);

				Update(); // 行列更新(フラグ変更後)

				// 親の情報を取得
				CalyxMath::Vector3 pScl = {
					CalyxMath::Vector3(parent->matrix.world.m[0][0], parent->matrix.world.m[0][1], parent->matrix.world.m[0][2]).Length(),
					CalyxMath::Vector3(parent->matrix.world.m[1][0], parent->matrix.world.m[1][1], parent->matrix.world.m[1][2]).Length(),
					CalyxMath::Vector3(parent->matrix.world.m[2][0], parent->matrix.world.m[2][1], parent->matrix.world.m[2][2]).Length()};
				CalyxMath::Matrix4x4 pRotMat = parent->matrix.world;
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
				CalyxMath::Vector3	  pPos							= {parent->matrix.world.m[3][0], parent->matrix.world.m[3][1], parent->matrix.world.m[3][2]};

				CalyxMath::Vector3	  effPScl = inheritScale ? pScl : CalyxMath::Vector3{1, 1, 1};
				CalyxMath::Quaternion effPRot = inheritRotate ? pRotQ : CalyxMath::Quaternion::MakeIdentity();
				CalyxMath::Vector3	  effPPos = inheritTranslate ? pPos : CalyxMath::Vector3{0, 0, 0};

				// ローカル値を逆算 (Rigid Inverse)
				if(std::abs(effPScl.x) > 0.0001f) scale.x = wScl.x / effPScl.x;
				if(std::abs(effPScl.y) > 0.0001f) scale.y = wScl.y / effPScl.y;
				if(std::abs(effPScl.z) > 0.0001f) scale.z = wScl.z / effPScl.z;

				rotation = CalyxMath::Quaternion::Multiply(wRotQ, CalyxMath::Quaternion::Inverse(effPRot));

				CalyxMath::Vector3 diff		  = {wPos.x - effPPos.x, wPos.y - effPPos.y, wPos.z - effPPos.z};
				CalyxMath::Vector3 localTrans = CalyxMath::Quaternion::RotateVector(diff, CalyxMath::Quaternion::Inverse(effPRot));
				if(std::abs(effPScl.x) > 0.0001f) localTrans.x /= effPScl.x;
				if(std::abs(effPScl.y) > 0.0001f) localTrans.y /= effPScl.y;
				if(std::abs(effPScl.z) > 0.0001f) localTrans.z /= effPScl.z;
				translation = localTrans;
			}
			ImGui::SameLine();
		}
		if(GuiCmd::ColoredDragFloat3("Scale", scale, 0.01f)) {}

		if(bInherit) {
			if(ImGui::Checkbox("##iRot", &inheritRotate)) {
				// iScaleと同様のFixup (全フラグに対して再計算が必要)
				ImGui::SetItemDefaultFocus(); // 無理やりトリガー
				bool dummy = true;
				if(dummy) {
					CalyxMath::Vector3 wPos = GetWorldPosition();
					CalyxMath::Vector3 wScl = {
						CalyxMath::Vector3(matrix.world.m[0][0], matrix.world.m[0][1], matrix.world.m[0][2]).Length(),
						CalyxMath::Vector3(matrix.world.m[1][0], matrix.world.m[1][1], matrix.world.m[1][2]).Length(),
						CalyxMath::Vector3(matrix.world.m[2][0], matrix.world.m[2][1], matrix.world.m[2][2]).Length()};
					CalyxMath::Matrix4x4 wRotMat = matrix.world;
					if(wScl.x > 0.0001f) {
						wRotMat.m[0][0] /= wScl.x;
						wRotMat.m[0][1] /= wScl.x;
						wRotMat.m[0][2] /= wScl.x;
					}
					if(wScl.y > 0.0001f) {
						wRotMat.m[1][0] /= wScl.y;
						wRotMat.m[1][1] /= wScl.y;
						wRotMat.m[1][2] /= wScl.y;
					}
					if(wScl.z > 0.0001f) {
						wRotMat.m[2][0] /= wScl.z;
						wRotMat.m[2][1] /= wScl.z;
						wRotMat.m[2][2] /= wScl.z;
					}
					wRotMat.m[3][0] = wRotMat.m[3][1] = wRotMat.m[3][2] = 0.0f;
					wRotMat.m[3][3]										= 1.0f;
					CalyxMath::Quaternion wRotQ							= CalyxMath::Quaternion::FromMatrix(wRotMat);

					Update();

					CalyxMath::Vector3 pScl = {
						CalyxMath::Vector3(parent->matrix.world.m[0][0], parent->matrix.world.m[0][1], parent->matrix.world.m[0][2]).Length(),
						CalyxMath::Vector3(parent->matrix.world.m[1][0], parent->matrix.world.m[1][1], parent->matrix.world.m[1][2]).Length(),
						CalyxMath::Vector3(parent->matrix.world.m[2][0], parent->matrix.world.m[2][1], parent->matrix.world.m[2][2]).Length()};
					CalyxMath::Matrix4x4 pRotMat = parent->matrix.world;
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
					CalyxMath::Vector3	  pPos							= {parent->matrix.world.m[3][0], parent->matrix.world.m[3][1], parent->matrix.world.m[3][2]};

					CalyxMath::Vector3	  effPScl = inheritScale ? pScl : CalyxMath::Vector3{1, 1, 1};
					CalyxMath::Quaternion effPRot = inheritRotate ? pRotQ : CalyxMath::Quaternion::MakeIdentity();
					CalyxMath::Vector3	  effPPos = inheritTranslate ? pPos : CalyxMath::Vector3{0, 0, 0};

					if(std::abs(effPScl.x) > 0.0001f) scale.x = wScl.x / effPScl.x;
					if(std::abs(effPScl.y) > 0.0001f) scale.y = wScl.y / effPScl.y;
					if(std::abs(effPScl.z) > 0.0001f) scale.z = wScl.z / effPScl.z;
					rotation					  = CalyxMath::Quaternion::Multiply(wRotQ, CalyxMath::Quaternion::Inverse(effPRot));
					CalyxMath::Vector3 diff		  = {wPos.x - effPPos.x, wPos.y - effPPos.y, wPos.z - effPPos.z};
					CalyxMath::Vector3 localTrans = CalyxMath::Quaternion::RotateVector(diff, CalyxMath::Quaternion::Inverse(effPRot));
					if(std::abs(effPScl.x) > 0.0001f) localTrans.x /= effPScl.x;
					if(std::abs(effPScl.y) > 0.0001f) localTrans.y /= effPScl.y;
					if(std::abs(effPScl.z) > 0.0001f) localTrans.z /= effPScl.z;
					translation = localTrans;
				}
			}
			ImGui::SameLine();
		}
		if(GuiCmd::ColoredDragFloat3("Rotation", eulerRotation, 0.1f, -360.0f, 360.0f, "%.1f", "°")) {
			rotationSource = RotationSource::Euler;
		}

		if(bInherit) {
			if(ImGui::Checkbox("##iTrans", &inheritTranslate)) {
				CalyxMath::Vector3 wPos = GetWorldPosition();
				CalyxMath::Vector3 wScl = {
					CalyxMath::Vector3(matrix.world.m[0][0], matrix.world.m[0][1], matrix.world.m[0][2]).Length(),
					CalyxMath::Vector3(matrix.world.m[1][0], matrix.world.m[1][1], matrix.world.m[1][2]).Length(),
					CalyxMath::Vector3(matrix.world.m[2][0], matrix.world.m[2][1], matrix.world.m[2][2]).Length()};
				CalyxMath::Matrix4x4 wRotMat = matrix.world;
				if(wScl.x > 0.0001f) {
					wRotMat.m[0][0] /= wScl.x;
					wRotMat.m[0][1] /= wScl.x;
					wRotMat.m[0][2] /= wScl.x;
				}
				if(wScl.y > 0.0001f) {
					wRotMat.m[1][0] /= wScl.y;
					wRotMat.m[1][1] /= wScl.y;
					wRotMat.m[1][2] /= wScl.y;
				}
				if(wScl.z > 0.0001f) {
					wRotMat.m[2][0] /= wScl.z;
					wRotMat.m[2][1] /= wScl.z;
					wRotMat.m[2][2] /= wScl.z;
				}
				wRotMat.m[3][0] = wRotMat.m[3][1] = wRotMat.m[3][2] = 0.0f;
				wRotMat.m[3][3]										= 1.0f;
				CalyxMath::Quaternion wRotQ							= CalyxMath::Quaternion::FromMatrix(wRotMat);

				Update();

				CalyxMath::Vector3 pScl = {
					CalyxMath::Vector3(parent->matrix.world.m[0][0], parent->matrix.world.m[0][1], parent->matrix.world.m[0][2]).Length(),
					CalyxMath::Vector3(parent->matrix.world.m[1][0], parent->matrix.world.m[1][1], parent->matrix.world.m[1][2]).Length(),
					CalyxMath::Vector3(parent->matrix.world.m[2][0], parent->matrix.world.m[2][1], parent->matrix.world.m[2][2]).Length()};
				CalyxMath::Matrix4x4 pRotMat = parent->matrix.world;
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
				CalyxMath::Vector3	  pPos							= {parent->matrix.world.m[3][0], parent->matrix.world.m[3][1], parent->matrix.world.m[3][2]};

				CalyxMath::Vector3	  effPScl = inheritScale ? pScl : CalyxMath::Vector3{1, 1, 1};
				CalyxMath::Quaternion effPRot = inheritRotate ? pRotQ : CalyxMath::Quaternion::MakeIdentity();
				CalyxMath::Vector3	  effPPos = inheritTranslate ? pPos : CalyxMath::Vector3{0, 0, 0};

				if(std::abs(effPScl.x) > 0.0001f) scale.x = wScl.x / effPScl.x;
				if(std::abs(effPScl.y) > 0.0001f) scale.y = wScl.y / effPScl.y;
				if(std::abs(effPScl.z) > 0.0001f) scale.z = wScl.z / effPScl.z;
				rotation					  = CalyxMath::Quaternion::Multiply(wRotQ, CalyxMath::Quaternion::Inverse(effPRot));
				CalyxMath::Vector3 diff		  = {wPos.x - effPPos.x, wPos.y - effPPos.y, wPos.z - effPPos.z};
				CalyxMath::Vector3 localTrans = CalyxMath::Quaternion::RotateVector(diff, CalyxMath::Quaternion::Inverse(effPRot));
				if(std::abs(effPScl.x) > 0.0001f) localTrans.x /= effPScl.x;
				if(std::abs(effPScl.y) > 0.0001f) localTrans.y /= effPScl.y;
				if(std::abs(effPScl.z) > 0.0001f) localTrans.z /= effPScl.z;
				translation = localTrans;
			}
			ImGui::SameLine();
		}
		if(GuiCmd::ColoredDragFloat3("Translation", translation, 0.01f)) {}

		ImGui::TreePop();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//	ワールド座標空間での位置を取得
/////////////////////////////////////////////////////////////////////////////////////////
CalyxMath::Vector3 BaseTransform::GetWorldPosition() const {
	CalyxMath::Vector3 worldPos{};
	worldPos.x = matrix.world.m[3][0];
	worldPos.y = matrix.world.m[3][1];
	worldPos.z = matrix.world.m[3][2];

	return worldPos;
}

/* ========================================================================
/* worldTransform class
/* ===================================================================== */
void WorldTransform::Update(const CalyxMath::Matrix4x4&) {
	Update();
}

/////////////////////////////////////////////////////////////////////////////////////////
//	worldTransformの更新
/////////////////////////////////////////////////////////////////////////////////////////
void WorldTransform::Update() {
	CalyxMath::Matrix4x4 scaleMat = CalyxMath::MakeScaleMatrix(scale);

	switch(rotationSource) {
	case RotationSource::Euler:
		rotation = CalyxMath::Quaternion::EulerToQuaternion(eulerRotation);
		break;
	case RotationSource::Quaternion:
		eulerRotation = CalyxMath::Quaternion::ToEuler(rotation);
		break;
	}

	CalyxMath::Matrix4x4 rotateMat	  = CalyxMath::Quaternion::ToMatrix(rotation);
	CalyxMath::Matrix4x4 translateMat = CalyxMath::MakeTranslateMatrix(translation);
	CalyxMath::Matrix4x4 localMat	  = scaleMat * rotateMat * translateMat;

	if(parent) {
		parent->Update();

		// 親のワールド情報
		CalyxMath::Vector3 pScl = {
			CalyxMath::Vector3(parent->matrix.world.m[0][0], parent->matrix.world.m[0][1], parent->matrix.world.m[0][2]).Length(),
			CalyxMath::Vector3(parent->matrix.world.m[1][0], parent->matrix.world.m[1][1], parent->matrix.world.m[1][2]).Length(),
			CalyxMath::Vector3(parent->matrix.world.m[2][0], parent->matrix.world.m[2][1], parent->matrix.world.m[2][2]).Length()};

		// 親の回転をとる (スケール除去済み行列から)
		CalyxMath::Matrix4x4 pRotMat = parent->matrix.world;
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

		CalyxMath::Vector3 pPos = {parent->matrix.world.m[3][0], parent->matrix.world.m[3][1], parent->matrix.world.m[3][2]};

		// 継承設定の適用
		CalyxMath::Vector3	  effPScl = inheritScale ? pScl : CalyxMath::Vector3{1, 1, 1};
		CalyxMath::Quaternion effPRot = inheritRotate ? pRotQ : CalyxMath::Quaternion::MakeIdentity();
		CalyxMath::Vector3	  effPPos = inheritTranslate ? pPos : CalyxMath::Vector3{0, 0, 0};

		// --- Rigid Component Reconstruction (歪みを除去するために個別合成) ---
		// 1. ワールドスケール
		CalyxMath::Vector3 worldScl = {scale.x * effPScl.x, scale.y * effPScl.y, scale.z * effPScl.z};
		// 2. ワールド回転 (local * parent)
		CalyxMath::Quaternion worldRot = CalyxMath::Quaternion::Multiply(rotation, effPRot);
		// 3. ワールド座標
		//    ParentPos + ParentRot * (ParentScale * LocalTranslation)
		CalyxMath::Vector3 scaledTrans	= {translation.x * effPScl.x, translation.y * effPScl.y, translation.z * effPScl.z};
		CalyxMath::Vector3 rotatedTrans = CalyxMath::Quaternion::RotateVector(scaledTrans, effPRot);
		CalyxMath::Vector3 worldPos		= {effPPos.x + rotatedTrans.x, effPPos.y + rotatedTrans.y, effPPos.z + rotatedTrans.z};

		// 4. 最終的なワールド行列の合成 (SRT) -> これで「せん断」が絶対に入らない綺麗な行列になる
		matrix.world = CalyxMath::MakeScaleMatrix(worldScl) * CalyxMath::Quaternion::ToMatrix(worldRot) * CalyxMath::MakeTranslateMatrix(worldPos);
	} else {
		// 親がいない場合は単純なSRT
		matrix.world = scaleMat * rotateMat * translateMat;
	}

	matrix.WorldInverseTranspose = CalyxMath::Matrix4x4::Transpose(CalyxMath::Matrix4x4::Inverse(matrix.world));
	TransferData(matrix);
}

/////////////////////////////////////////////////////////////////////////////////////////
//	継承設定を考慮した親行列を取得
/////////////////////////////////////////////////////////////////////////////////////////
CalyxMath::Matrix4x4 WorldTransform::GetEffectiveParentMatrix() const {
	if(!parent) {
		return CalyxMath::Matrix4x4::MakeIdentity();
	}

	CalyxMath::Matrix4x4 pMat	= parent->matrix.world;
	CalyxMath::Vector3	 pTrans = {pMat.m[3][0], pMat.m[3][1], pMat.m[3][2]};

	float			   pSclX = CalyxMath::Vector3(pMat.m[0][0], pMat.m[0][1], pMat.m[0][2]).Length();
	float			   pSclY = CalyxMath::Vector3(pMat.m[1][0], pMat.m[1][1], pMat.m[1][2]).Length();
	float			   pSclZ = CalyxMath::Vector3(pMat.m[2][0], pMat.m[2][1], pMat.m[2][2]).Length();
	CalyxMath::Vector3 pScl	 = {pSclX, pSclY, pSclZ};

	CalyxMath::Matrix4x4 pRotMat = pMat;
	if(pSclX > 0.0001f) {
		pRotMat.m[0][0] /= pSclX;
		pRotMat.m[0][1] /= pSclX;
		pRotMat.m[0][2] /= pSclX;
	}
	if(pSclY > 0.0001f) {
		pRotMat.m[1][0] /= pSclY;
		pRotMat.m[1][1] /= pSclY;
		pRotMat.m[1][2] /= pSclY;
	}
	if(pSclZ > 0.0001f) {
		pRotMat.m[2][0] /= pSclZ;
		pRotMat.m[2][1] /= pSclZ;
		pRotMat.m[2][2] /= pSclZ;
	}
	pRotMat.m[3][0] = 0.0f;
	pRotMat.m[3][1] = 0.0f;
	pRotMat.m[3][2] = 0.0f;
	pRotMat.m[3][3] = 1.0f;

	// 継承設定の適用
	CalyxMath::Vector3	 effectiveScl = inheritScale ? pScl : CalyxMath::Vector3{1.0f, 1.0f, 1.0f};
	CalyxMath::Matrix4x4 effectiveRot = inheritRotate ? pRotMat : CalyxMath::Matrix4x4::MakeIdentity();
	CalyxMath::Vector3	 effectivePos = inheritTranslate ? pTrans : CalyxMath::Vector3{0.0f, 0.0f, 0.0f};

	return CalyxMath::MakeScaleMatrix(effectiveScl) * effectiveRot * CalyxMath::MakeTranslateMatrix(effectivePos);
}

CalyxMath::Vector3 WorldTransform::GetForward() const {
	// ワールド行列のZ軸（前方向）
	CalyxMath::Matrix4x4 mat	 = CalyxMath::MakeAffineMatrix(scale, rotation, translation);
	CalyxMath::Vector3	 forward = {mat.m[2][0], mat.m[2][1], mat.m[2][2]};
	return forward.Normalize();
}

/////////////////////////////////////////////////////////////////////////////////////////
//	コンフィグ適用
/////////////////////////////////////////////////////////////////////////////////////////
void WorldTransform::ApplyConfig(const WorldTransformConfig& config) {
	scale		= config.scale;
	translation = config.translation;
	rotation	= config.rotation;

	inheritScale	 = config.inheritScale;
	inheritRotate	 = config.inheritRotate;
	inheritTranslate = config.inheritTranslate;

	eulerRotation  = CalyxMath::Quaternion::ToEuler(rotation);
	rotationSource = RotationSource::Quaternion;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	コンフィグから抽出
/////////////////////////////////////////////////////////////////////////////////////////
WorldTransformConfig WorldTransform::ExtractConfig() {
	WorldTransformConfig config;
	config.translation = translation;

	if(rotationSource == RotationSource::Euler) {
		config.rotation = CalyxMath::Quaternion::EulerToQuaternion(eulerRotation);
	} else {
		config.rotation = rotation;
	}

	config.scale			= scale;
	config.inheritScale		= inheritScale;
	config.inheritRotate	= inheritRotate;
	config.inheritTranslate = inheritTranslate;
	return config;
}

CalyxMath::Matrix4x4 Transform2D::GetMatrix() const {
	CalyxMath::Matrix4x4 matWorld =
		MakeAffineMatrix(
			{scale.x, scale.y, 1.0f},
			{0, 0, rotate},
			{translate.x, translate.y, 0.0f});

	CalyxMath::Matrix4x4 matView = CalyxMath::Matrix4x4::MakeIdentity();
	CalyxMath::Matrix4x4 matProj = MakeOrthographicMatrix(
		0.0f, 0.0f,
		kWindowWidth, kWindowHeight,
		0.0f, 100.0f);

	return CalyxMath::Matrix4x4::Multiply(matWorld, CalyxMath::Matrix4x4::Multiply(matView, matProj));
}

/* ========================================================================
/* Transform2D class
/* ===================================================================== */
void Transform2D::ShowImGui(const std::string& lavel) {
	std::string nodeLabel = lavel + "_tabbar";

	if(ImGui::TreeNodeEx(nodeLabel.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
		if(GuiCmd::DragFloat2("scale", scale, 0.01f)) {}

		if(GuiCmd::DragFloat("rotation", rotate, 0.01f)) {}

		if(GuiCmd::DragFloat2("translate", translate, 0.01f)) {}

		ImGui::TreePop();
	}
}

Transform2DConfig Transform2D::ExtractConfig() const {
	Transform2DConfig config;
	config.scale	   = scale;
	config.rotation	   = rotate;
	config.translation = translate;
	return config;
}

void Transform2D::ShowImGui(Transform2DConfig& config, const std::string& lavel) {
	if(ImGui::TreeNodeEx(lavel.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
		if(GuiCmd::DragFloat2("scale", config.scale, 0.01f)) {}

		if(GuiCmd::DragFloat("rotation", config.rotation, 0.01f)) {}

		if(GuiCmd::DragFloat2("translate", config.translation, 0.01f)) {}
		ImGui::TreePop();
	}
}

void Transform2D::ApplyConfig(const Transform2DConfig& config) {
	scale	  = config.scale;
	rotate	  = config.rotation;
	translate = config.translation;
}