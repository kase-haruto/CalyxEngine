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
#include "Engine/Foundation/Math/MathUtil.h"

#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <externals/imgui/imgui.h>

using namespace CalyxEngine;

namespace {
	bool SameQuaternion(const CalyxEngine::Quaternion& lhs, const CalyxEngine::Quaternion& rhs) {
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
	}
}

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

	revision_ = 1;
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

	//eulerとquauternionどちらの回転をさせているのか表示
	rotationSource == RotationSource::Euler ? ImGui::Text("Rotation: Euler") : ImGui::Text("Rotation: Quaternion");

	std::string nodeLabel = label + "##node";
	// 小さめの折りたたみ見出しとして TreeNodeEx を使用
	if(ImGui::TreeNodeEx(nodeLabel.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
		bool bInherit = parent != nullptr;

		if(bInherit) {
			if(ImGui::Checkbox("##iScale", &inheritScale)) {
				// 継承フラグが切り替わった瞬間に、現在のワールド状態を維持するようにローカル値を逆算(Fixup)
				CalyxEngine::Vector3 wPos = GetWorldPosition();
				CalyxEngine::Vector3 wScl = {
					CalyxEngine::Vector3(matrix.world.m[0][0], matrix.world.m[0][1], matrix.world.m[0][2]).Length(),
					CalyxEngine::Vector3(matrix.world.m[1][0], matrix.world.m[1][1], matrix.world.m[1][2]).Length(),
					CalyxEngine::Vector3(matrix.world.m[2][0], matrix.world.m[2][1], matrix.world.m[2][2]).Length()};
				CalyxEngine::Matrix4x4 wRotMat = matrix.world;
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
				CalyxEngine::Quaternion wRotQ							= CalyxEngine::Quaternion::FromMatrix(wRotMat);

				Update(); // 行列更新(フラグ変更後)

				// 親の情報を取得
				CalyxEngine::Vector3 pScl = {
					CalyxEngine::Vector3(parent->matrix.world.m[0][0], parent->matrix.world.m[0][1], parent->matrix.world.m[0][2]).Length(),
					CalyxEngine::Vector3(parent->matrix.world.m[1][0], parent->matrix.world.m[1][1], parent->matrix.world.m[1][2]).Length(),
					CalyxEngine::Vector3(parent->matrix.world.m[2][0], parent->matrix.world.m[2][1], parent->matrix.world.m[2][2]).Length()};
				CalyxEngine::Matrix4x4 pRotMat = parent->matrix.world;
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
				CalyxEngine::Quaternion pRotQ							= CalyxEngine::Quaternion::FromMatrix(pRotMat);
				CalyxEngine::Vector3	  pPos							= {parent->matrix.world.m[3][0], parent->matrix.world.m[3][1], parent->matrix.world.m[3][2]};

				CalyxEngine::Vector3	  effPScl = inheritScale ? pScl : CalyxEngine::Vector3{1, 1, 1};
				CalyxEngine::Quaternion effPRot = inheritRotate ? pRotQ : CalyxEngine::Quaternion::MakeIdentity();
				CalyxEngine::Vector3	  effPPos = inheritTranslate ? pPos : CalyxEngine::Vector3{0, 0, 0};

				// ローカル値を逆算 (Rigid Inverse)
				if(std::abs(effPScl.x) > 0.0001f) scale.x = wScl.x / effPScl.x;
				if(std::abs(effPScl.y) > 0.0001f) scale.y = wScl.y / effPScl.y;
				if(std::abs(effPScl.z) > 0.0001f) scale.z = wScl.z / effPScl.z;

				rotation = CalyxEngine::Quaternion::Multiply(wRotQ, CalyxEngine::Quaternion::Inverse(effPRot));

				CalyxEngine::Vector3 diff		  = {wPos.x - effPPos.x, wPos.y - effPPos.y, wPos.z - effPPos.z};
				CalyxEngine::Vector3 localTrans = CalyxEngine::Quaternion::RotateVector(diff, CalyxEngine::Quaternion::Inverse(effPRot));
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
					CalyxEngine::Vector3 wPos = GetWorldPosition();
					CalyxEngine::Vector3 wScl = {
						CalyxEngine::Vector3(matrix.world.m[0][0], matrix.world.m[0][1], matrix.world.m[0][2]).Length(),
						CalyxEngine::Vector3(matrix.world.m[1][0], matrix.world.m[1][1], matrix.world.m[1][2]).Length(),
						CalyxEngine::Vector3(matrix.world.m[2][0], matrix.world.m[2][1], matrix.world.m[2][2]).Length()};
					CalyxEngine::Matrix4x4 wRotMat = matrix.world;
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
					CalyxEngine::Quaternion wRotQ							= CalyxEngine::Quaternion::FromMatrix(wRotMat);

					Update();

					CalyxEngine::Vector3 pScl = {
						CalyxEngine::Vector3(parent->matrix.world.m[0][0], parent->matrix.world.m[0][1], parent->matrix.world.m[0][2]).Length(),
						CalyxEngine::Vector3(parent->matrix.world.m[1][0], parent->matrix.world.m[1][1], parent->matrix.world.m[1][2]).Length(),
						CalyxEngine::Vector3(parent->matrix.world.m[2][0], parent->matrix.world.m[2][1], parent->matrix.world.m[2][2]).Length()};
					CalyxEngine::Matrix4x4 pRotMat = parent->matrix.world;
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
					CalyxEngine::Quaternion pRotQ							= CalyxEngine::Quaternion::FromMatrix(pRotMat);
					CalyxEngine::Vector3	  pPos							= {parent->matrix.world.m[3][0], parent->matrix.world.m[3][1], parent->matrix.world.m[3][2]};

					CalyxEngine::Vector3	  effPScl = inheritScale ? pScl : CalyxEngine::Vector3{1, 1, 1};
					CalyxEngine::Quaternion effPRot = inheritRotate ? pRotQ : CalyxEngine::Quaternion::MakeIdentity();
					CalyxEngine::Vector3	  effPPos = inheritTranslate ? pPos : CalyxEngine::Vector3{0, 0, 0};

					if(std::abs(effPScl.x) > 0.0001f) scale.x = wScl.x / effPScl.x;
					if(std::abs(effPScl.y) > 0.0001f) scale.y = wScl.y / effPScl.y;
					if(std::abs(effPScl.z) > 0.0001f) scale.z = wScl.z / effPScl.z;
					rotation					  = CalyxEngine::Quaternion::Multiply(wRotQ, CalyxEngine::Quaternion::Inverse(effPRot));
					CalyxEngine::Vector3 diff		  = {wPos.x - effPPos.x, wPos.y - effPPos.y, wPos.z - effPPos.z};
					CalyxEngine::Vector3 localTrans = CalyxEngine::Quaternion::RotateVector(diff, CalyxEngine::Quaternion::Inverse(effPRot));
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
				CalyxEngine::Vector3 wPos = GetWorldPosition();
				CalyxEngine::Vector3 wScl = {
					CalyxEngine::Vector3(matrix.world.m[0][0], matrix.world.m[0][1], matrix.world.m[0][2]).Length(),
					CalyxEngine::Vector3(matrix.world.m[1][0], matrix.world.m[1][1], matrix.world.m[1][2]).Length(),
					CalyxEngine::Vector3(matrix.world.m[2][0], matrix.world.m[2][1], matrix.world.m[2][2]).Length()};
				CalyxEngine::Matrix4x4 wRotMat = matrix.world;
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
				CalyxEngine::Quaternion wRotQ							= CalyxEngine::Quaternion::FromMatrix(wRotMat);

				Update();

				CalyxEngine::Vector3 pScl = {
					CalyxEngine::Vector3(parent->matrix.world.m[0][0], parent->matrix.world.m[0][1], parent->matrix.world.m[0][2]).Length(),
					CalyxEngine::Vector3(parent->matrix.world.m[1][0], parent->matrix.world.m[1][1], parent->matrix.world.m[1][2]).Length(),
					CalyxEngine::Vector3(parent->matrix.world.m[2][0], parent->matrix.world.m[2][1], parent->matrix.world.m[2][2]).Length()};
				CalyxEngine::Matrix4x4 pRotMat = parent->matrix.world;
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
				CalyxEngine::Quaternion pRotQ							= CalyxEngine::Quaternion::FromMatrix(pRotMat);
				CalyxEngine::Vector3	  pPos							= {parent->matrix.world.m[3][0], parent->matrix.world.m[3][1], parent->matrix.world.m[3][2]};

				CalyxEngine::Vector3	  effPScl = inheritScale ? pScl : CalyxEngine::Vector3{1, 1, 1};
				CalyxEngine::Quaternion effPRot = inheritRotate ? pRotQ : CalyxEngine::Quaternion::MakeIdentity();
				CalyxEngine::Vector3	  effPPos = inheritTranslate ? pPos : CalyxEngine::Vector3{0, 0, 0};

				if(std::abs(effPScl.x) > 0.0001f) scale.x = wScl.x / effPScl.x;
				if(std::abs(effPScl.y) > 0.0001f) scale.y = wScl.y / effPScl.y;
				if(std::abs(effPScl.z) > 0.0001f) scale.z = wScl.z / effPScl.z;
				rotation					  = CalyxEngine::Quaternion::Multiply(wRotQ, CalyxEngine::Quaternion::Inverse(effPRot));
				CalyxEngine::Vector3 diff		  = {wPos.x - effPPos.x, wPos.y - effPPos.y, wPos.z - effPPos.z};
				CalyxEngine::Vector3 localTrans = CalyxEngine::Quaternion::RotateVector(diff, CalyxEngine::Quaternion::Inverse(effPRot));
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
CalyxEngine::Vector3 BaseTransform::GetWorldPosition() const {
	CalyxEngine::Vector3 worldPos{};
	worldPos.x = matrix.world.m[3][0];
	worldPos.y = matrix.world.m[3][1];
	worldPos.z = matrix.world.m[3][2];

	return worldPos;
}

/* ========================================================================
/* worldTransform class
/* ===================================================================== */
void WorldTransform::Update(const CalyxEngine::Matrix4x4&) {
	Update();
}

/////////////////////////////////////////////////////////////////////////////////////////
//	worldTransformの更新
/////////////////////////////////////////////////////////////////////////////////////////
void WorldTransform::Update() {
	uint64_t parentRevision = 0;
	if(parent) {
		parent->Update();
		parentRevision = parent->GetRevision();
	}

	if(IsCacheValid(parentRevision)) {
		return;
	}

	CalyxEngine::Matrix4x4 scaleMat = CalyxEngine::MakeScaleMatrix(scale);

	switch(rotationSource) {
	case RotationSource::Euler:
		rotation = CalyxEngine::Quaternion::EulerToQuaternion(eulerRotation);
		break;
	case RotationSource::Quaternion:
		eulerRotation = CalyxEngine::Quaternion::ToEuler(rotation);
		break;
	}

	CalyxEngine::Matrix4x4 rotateMat	  = CalyxEngine::Quaternion::ToMatrix(rotation);
	CalyxEngine::Matrix4x4 translateMat = CalyxEngine::MakeTranslateMatrix(translation);
	CalyxEngine::Matrix4x4 localMat	  = scaleMat * rotateMat * translateMat;

	if(parent) {
		// 親のワールド情報
		CalyxEngine::Vector3 pScl = {
			CalyxEngine::Vector3(parent->matrix.world.m[0][0], parent->matrix.world.m[0][1], parent->matrix.world.m[0][2]).Length(),
			CalyxEngine::Vector3(parent->matrix.world.m[1][0], parent->matrix.world.m[1][1], parent->matrix.world.m[1][2]).Length(),
			CalyxEngine::Vector3(parent->matrix.world.m[2][0], parent->matrix.world.m[2][1], parent->matrix.world.m[2][2]).Length()};

		// 親の回転をとる (スケール除去済み行列から)
		CalyxEngine::Matrix4x4 pRotMat = parent->matrix.world;
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
		CalyxEngine::Quaternion pRotQ							= CalyxEngine::Quaternion::FromMatrix(pRotMat);

		CalyxEngine::Vector3 pPos = {parent->matrix.world.m[3][0], parent->matrix.world.m[3][1], parent->matrix.world.m[3][2]};

		// 継承設定の適用
		CalyxEngine::Vector3	  effPScl = inheritScale ? pScl : CalyxEngine::Vector3{1, 1, 1};
		CalyxEngine::Quaternion effPRot = inheritRotate ? pRotQ : CalyxEngine::Quaternion::MakeIdentity();
		CalyxEngine::Vector3	  effPPos = inheritTranslate ? pPos : CalyxEngine::Vector3{0, 0, 0};

		// --- Rigid Component Reconstruction (歪みを除去するために個別合成) ---
		// 1. ワールドスケール
		CalyxEngine::Vector3 worldScl = {scale.x * effPScl.x, scale.y * effPScl.y, scale.z * effPScl.z};
		// 2. ワールド回転 (local * parent)
		CalyxEngine::Quaternion worldRot = CalyxEngine::Quaternion::Multiply(rotation, effPRot);
		// 3. ワールド座標
		//    ParentPos + ParentRot * (ParentScale * LocalTranslation)
		CalyxEngine::Vector3 scaledTrans	= {translation.x * effPScl.x, translation.y * effPScl.y, translation.z * effPScl.z};
		CalyxEngine::Vector3 rotatedTrans = CalyxEngine::Quaternion::RotateVector(scaledTrans, effPRot);
		CalyxEngine::Vector3 worldPos		= {effPPos.x + rotatedTrans.x, effPPos.y + rotatedTrans.y, effPPos.z + rotatedTrans.z};

		// 4. 最終的なワールド行列の合成 (SRT) -> これで「せん断」が絶対に入らない綺麗な行列になる
		matrix.world = CalyxEngine::MakeScaleMatrix(worldScl) * CalyxEngine::Quaternion::ToMatrix(worldRot) * CalyxEngine::MakeTranslateMatrix(worldPos);
	} else {
		// 親がいない場合は単純なSRT
		matrix.world = scaleMat * rotateMat * translateMat;
	}

	matrix.WorldInverseTranspose = CalyxEngine::Matrix4x4::Transpose(CalyxEngine::Matrix4x4::Inverse(matrix.world));
	TransferData(matrix);
	StoreCache(parentRevision);
	++revision_;
}

bool WorldTransform::IsCacheValid(uint64_t parentRevision) const {
	if(!cacheValid_) return false;
	if(parent != cachedParent_) return false;
	if(parentRevision != cachedParentRevision_) return false;
	if(scale != cachedScale_) return false;
	if(translation != cachedTranslation_) return false;
	if(rotationSource != cachedRotationSource_) return false;
	if(eulerRotation != cachedEulerRotation_) return false;
	if(!SameQuaternion(rotation, cachedRotation_)) return false;
	if(inheritTranslate != cachedInheritTranslate_) return false;
	if(inheritRotate != cachedInheritRotate_) return false;
	if(inheritScale != cachedInheritScale_) return false;
	return true;
}

void WorldTransform::StoreCache(uint64_t parentRevision) {
	cachedScale_			   = scale;
	cachedRotation_			   = rotation;
	cachedEulerRotation_	   = eulerRotation;
	cachedTranslation_		   = translation;
	cachedParent_			   = parent;
	cachedParentRevision_	   = parentRevision;
	cachedRotationSource_	   = rotationSource;
	cachedInheritTranslate_	   = inheritTranslate;
	cachedInheritRotate_	   = inheritRotate;
	cachedInheritScale_		   = inheritScale;
	cacheValid_				   = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	継承設定を考慮した親行列を取得
/////////////////////////////////////////////////////////////////////////////////////////
CalyxEngine::Matrix4x4 WorldTransform::GetEffectiveParentMatrix() const {
	if(!parent) {
		return CalyxEngine::Matrix4x4::MakeIdentity();
	}

	CalyxEngine::Matrix4x4 pMat	= parent->matrix.world;
	CalyxEngine::Vector3	 pTrans = {pMat.m[3][0], pMat.m[3][1], pMat.m[3][2]};

	float			   pSclX = CalyxEngine::Vector3(pMat.m[0][0], pMat.m[0][1], pMat.m[0][2]).Length();
	float			   pSclY = CalyxEngine::Vector3(pMat.m[1][0], pMat.m[1][1], pMat.m[1][2]).Length();
	float			   pSclZ = CalyxEngine::Vector3(pMat.m[2][0], pMat.m[2][1], pMat.m[2][2]).Length();
	CalyxEngine::Vector3 pScl	 = {pSclX, pSclY, pSclZ};

	CalyxEngine::Matrix4x4 pRotMat = pMat;
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
	CalyxEngine::Vector3	 effectiveScl = inheritScale ? pScl : CalyxEngine::Vector3{1.0f, 1.0f, 1.0f};
	CalyxEngine::Matrix4x4 effectiveRot = inheritRotate ? pRotMat : CalyxEngine::Matrix4x4::MakeIdentity();
	CalyxEngine::Vector3	 effectivePos = inheritTranslate ? pTrans : CalyxEngine::Vector3{0.0f, 0.0f, 0.0f};

	return CalyxEngine::MakeScaleMatrix(effectiveScl) * effectiveRot * CalyxEngine::MakeTranslateMatrix(effectivePos);
}

CalyxEngine::Vector3 WorldTransform::GetForward() const {
	// ワールド行列のZ軸（前方向）
	CalyxEngine::Matrix4x4 mat	 = CalyxEngine::MakeAffineMatrix(scale, rotation, translation);
	CalyxEngine::Vector3	 forward = {mat.m[2][0], mat.m[2][1], mat.m[2][2]};
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

	eulerRotation  = CalyxEngine::Quaternion::ToEuler(rotation);
	rotationSource = RotationSource::Quaternion;
	cacheValid_	   = false;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	コンフィグから抽出
/////////////////////////////////////////////////////////////////////////////////////////
WorldTransformConfig WorldTransform::ExtractConfig() {
	WorldTransformConfig config;
	config.translation = translation;

	if(rotationSource == RotationSource::Euler) {
		config.rotation = CalyxEngine::Quaternion::EulerToQuaternion(eulerRotation);
	} else {
		config.rotation = rotation;
	}

	config.scale			= scale;
	config.inheritScale		= inheritScale;
	config.inheritRotate	= inheritRotate;
	config.inheritTranslate = inheritTranslate;
	return config;
}

CalyxEngine::Matrix4x4 Transform2D::GetMatrix() const {
	CalyxEngine::Matrix4x4 matWorld =
		MakeAffineMatrix(
			{scale.x, scale.y, 1.0f},
			{0, 0, rotate},
			{translate.x, translate.y, 0.0f});

	CalyxEngine::Matrix4x4 matView = CalyxEngine::Matrix4x4::MakeIdentity();
	CalyxEngine::Matrix4x4 matProj = MakeOrthographicMatrix(
		0.0f, 0.0f,
		kWindowWidth, kWindowHeight,
		0.0f, 100.0f);

	return CalyxEngine::Matrix4x4::Multiply(matWorld, CalyxEngine::Matrix4x4::Multiply(matView, matProj));
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