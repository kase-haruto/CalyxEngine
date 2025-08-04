#include "Transform.h"
/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/Graphics/Context/GraphicsGroup.h>

//data
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

// lib
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <externals/imgui/imgui.h>
#include <Engine/Foundation/Json/JsonUtils.h>


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
	scale.Initialize({ 1.0f, 1.0f, 1.0f });
	rotation.Initialize();
	translation.Initialize();

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
Vector3 BaseTransform::GetWorldPosition() const {
	Vector3 worldPos{};
	worldPos.x = matrix.world.m[3][0];
	worldPos.y = matrix.world.m[3][1];
	worldPos.z = matrix.world.m[3][2];

	return worldPos;
}

/* ========================================================================
/* worldTransform class
/* ===================================================================== */
/////////////////////////////////////////////////////////////////////////////////////////
//	worldTransformの更新
/////////////////////////////////////////////////////////////////////////////////////////
void WorldTransform::Update(const Matrix4x4& viewProjMatrix) {
	Matrix4x4 scaleMat = MakeScaleMatrix(scale);

	// どちらをソースとするかで処理を分ける
	if (rotationSource == RotationSource::Euler) {
		rotation = Quaternion::EulerToQuaternion(eulerRotation);
	} else if (rotationSource == RotationSource::Quaternion) {
		eulerRotation = Quaternion::ToEuler(rotation);
	}

	Matrix4x4 rotateMat = Quaternion::ToMatrix(rotation);
	Matrix4x4 translateMat = MakeTranslateMatrix(translation);

	Matrix4x4 localMat = scaleMat * rotateMat * translateMat;

	if (parent) {
		matrix.world = localMat * parent->matrix.world;
	} else {
		matrix.world = localMat;
	}

	matrix.WVP = matrix.world * viewProjMatrix;
	matrix.WorldInverseTranspose = Matrix4x4::Transpose(Matrix4x4::Inverse(matrix.world));

	TransferData(matrix);
}

/////////////////////////////////////////////////////////////////////////////////////////
//	worldTransformの更新(カメラなし
/////////////////////////////////////////////////////////////////////////////////////////
void WorldTransform::Update() {
	Matrix4x4 scaleMat = MakeScaleMatrix(scale);

	// 回転の更新（オイラー角 ↔ クォータニオン 双方向変換）
	switch (rotationSource) {
		case RotationSource::Euler:
			rotation = Quaternion::EulerToQuaternion(eulerRotation);
			break;
		case RotationSource::Quaternion:
			eulerRotation = Quaternion::ToEuler(rotation);
			break;
	}

	Matrix4x4 rotateMat = Quaternion::ToMatrix(rotation);
	Matrix4x4 translateMat = MakeTranslateMatrix(translation);

	Matrix4x4 localMat = scaleMat * rotateMat * translateMat;

	if (parent) {
		matrix.world = localMat * parent->matrix.world;
	} else {
		matrix.world = localMat;
	}

	matrix.WorldInverseTranspose = Matrix4x4::Transpose(Matrix4x4::Inverse(matrix.world));

	TransferData(matrix);
}


/////////////////////////////////////////////////////////////////////////////////////////
//	コンフィグ適用
/////////////////////////////////////////////////////////////////////////////////////////
void WorldTransform::ApplyConfig(const WorldTransformConfig& config) {
	scale = config.scale;
	translation = config.translation;
	rotation = config.rotation;

	eulerRotation = Quaternion::ToEuler(rotation);
	rotationSource = RotationSource::Quaternion;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	コンフィグから抽出
/////////////////////////////////////////////////////////////////////////////////////////
WorldTransformConfig WorldTransform::ExtractConfig() {
	WorldTransformConfig config;
	config.translation = translation;
	config.rotation = rotation;
	config.scale = scale;
	return config;
}


/* ========================================================================
/* Transform2D class
/* ===================================================================== */
void Transform2D::ShowImGui(const std::string& lavel) {
	if (ImGui::CollapsingHeader(lavel.c_str())) {
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
	if (ImGui::CollapsingHeader(lavel.c_str())) {
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
