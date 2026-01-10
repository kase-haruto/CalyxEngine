#include "Transform.h"
/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Foundation/Utility/Func/CxUtils.h>

//data
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

// lib
#include "Engine/Application/System/Enviroment.h"

#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <externals/imgui/imgui.h>

using namespace CalyxMath;

/////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
void EulerTransform::ShowImGui(const std::string& label) {
	ImGui::SeparatorText(label.c_str());
	std::string scaleLabel = label + "_scale";
	std::string rotationLabel = label + "_rotation";
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

	//バッファの作成
	DxConstantBuffer::Initialize(GraphicsGroup::GetInstance()->GetDevice());

	Update();
}

/////////////////////////////////////////////////////////////////////////////////////////
//	imgui
/////////////////////////////////////////////////////////////////////////////////////////
void BaseTransform::ShowImGui(const std::string& label){
	if (ImGui::CollapsingHeader(label.c_str())){
		if (GuiCmd::ColoredDragFloat3("Scale", scale, 0.01f)){
		}

		if (GuiCmd::ColoredDragFloat3("Rotation", eulerRotation, 0.1f, -360.0f, 360.0f, "%.1f", "°")){
			rotationSource = RotationSource::Euler;
		}

		if (GuiCmd::ColoredDragFloat3("Translation", translation, 0.01f)){
		}
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
void WorldTransform::Update([[maybe_unused]]const CalyxMath::Matrix4x4& viewProjMatrix) {
	CalyxMath::Matrix4x4 scaleMat = CalyxMath::MakeScaleMatrix(scale);

	// どちらをソースとするかで処理を分ける
	if (rotationSource == RotationSource::Euler) {
		rotation = CalyxMath::Quaternion::EulerToQuaternion(eulerRotation);
	} else if (rotationSource == RotationSource::Quaternion) {
		eulerRotation = CalyxMath::Quaternion::ToEuler(rotation);
	}

	CalyxMath::Matrix4x4 rotateMat = CalyxMath::Quaternion::ToMatrix(rotation);
	CalyxMath::Matrix4x4 translateMat = CalyxMath::MakeTranslateMatrix(translation);

	CalyxMath::Matrix4x4 localMat = scaleMat * rotateMat * translateMat;

	if (parent) {
		parent->Update();
		matrix.world = localMat * parent->matrix.world;
	} else {
		matrix.world = localMat;
	}

	matrix.WorldInverseTranspose = CalyxMath::Matrix4x4::Transpose(CalyxMath::Matrix4x4::Inverse(matrix.world));

	TransferData(matrix);
}

/////////////////////////////////////////////////////////////////////////////////////////
//	worldTransformの更新(カメラなし
/////////////////////////////////////////////////////////////////////////////////////////
void WorldTransform::Update() {
	CalyxMath::Matrix4x4 scaleMat = CalyxMath::MakeScaleMatrix(scale);

	switch (rotationSource) {
	case RotationSource::Euler:
		rotation = CalyxMath::Quaternion::EulerToQuaternion(eulerRotation);
		break;
	case RotationSource::Quaternion:
		eulerRotation = CalyxMath::Quaternion::ToEuler(rotation);
		break;
	}

	CalyxMath::Matrix4x4 rotateMat	   = CalyxMath::Quaternion::ToMatrix(rotation);
	CalyxMath::Matrix4x4 translateMat = CalyxMath::MakeTranslateMatrix(translation);
	CalyxMath::Matrix4x4 localMat	   = scaleMat * rotateMat * translateMat;

	if (parent) {
		parent->Update();

		if (inheritScale) {
			matrix.world = localMat * parent->matrix.world;
		} else {
			// 親スケールを無視（親の回転＋平行移動のみを手動合成）
			CalyxMath::Matrix4x4 parentRotMat   = CalyxMath::Quaternion::ToMatrix(parent->rotation);
			CalyxMath::Matrix4x4 parentTransMat = CalyxMath::MakeTranslateMatrix(parent->translation);
			CalyxMath::Matrix4x4 parentNoScaleMat = parentRotMat * parentTransMat;

			matrix.world = localMat * parentNoScaleMat;
		}
	} else {
		matrix.world = localMat;
	}

	matrix.WorldInverseTranspose = CalyxMath::Matrix4x4::Transpose(CalyxMath::Matrix4x4::Inverse(matrix.world));
	TransferData(matrix);
}


CalyxMath::Vector3 WorldTransform::GetForward() const {
	// ワールド行列のZ軸（前方向）
	CalyxMath::Matrix4x4 mat = CalyxMath::MakeAffineMatrix(scale, rotation, translation);
	CalyxMath::Vector3 forward = { mat.m[2][0], mat.m[2][1], mat.m[2][2] };
	return forward.Normalize();
}

/////////////////////////////////////////////////////////////////////////////////////////
//	コンフィグ適用
/////////////////////////////////////////////////////////////////////////////////////////
void WorldTransform::ApplyConfig(const WorldTransformConfig& config) {
	scale = config.scale;
	translation = config.translation;
	rotation = config.rotation;

	eulerRotation = CalyxMath::Quaternion::ToEuler(rotation);
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

	config.scale = scale;
	return config;
}

CalyxMath::Matrix4x4 Transform2D::GetMatrix() const {
	CalyxMath::Matrix4x4 matWorld =
		MakeAffineMatrix(
			{scale.x, scale.y, 1.0f},
			{0, 0, rotate},
			{translate.x, translate.y, 0.0f}
		);

	CalyxMath::Matrix4x4 matView = CalyxMath::Matrix4x4::MakeIdentity();
	CalyxMath::Matrix4x4 matProj = MakeOrthographicMatrix(
		0.0f, 0.0f,
		kWindowWidth, kWindowHeight,
		0.0f, 100.0f
	);

	return CalyxMath::Matrix4x4::Multiply(matWorld, CalyxMath::Matrix4x4::Multiply(matView, matProj));
}
/* ========================================================================
/* Transform2D class
/* ===================================================================== */
void Transform2D::ShowImGui(const std::string& lavel) {
	if (GuiCmd::CollapsingHeader(lavel.c_str())) {
		GuiCmd::DragFloat2("scale", scale, 0.01f);
		GuiCmd::DragFloat("rotation", rotate, 0.01f);
		GuiCmd::DragFloat2("translate", translate, 0.01f);
	}
}

Transform2DConfig Transform2D::ExtractConfig() const {
	Transform2DConfig config;
	config.scale = scale;
	config.rotation = rotate;
	config.translation = translate;
	return config;
}

void Transform2D::ShowImGui(Transform2DConfig& config, const std::string& lavel) {
	if (GuiCmd::CollapsingHeader(lavel.c_str())) {
		GuiCmd::DragFloat2("scale", config.scale, 0.01f);
		GuiCmd::DragFloat("rotation", config.rotation, 0.01f);
		GuiCmd::DragFloat2("translate", config.translation, 0.01f);
	}
}

void Transform2D::ApplyConfig(const Transform2DConfig& config) {
	scale = config.scale;
	rotate = config.rotation;
	translate = config.translation;
}