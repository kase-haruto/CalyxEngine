#pragma once
//===================================================================*/
//				incalue space
//===================================================================*/
// math
#include <Engine/Foundation/Math/Mat3.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

// engine 
#include <Engine/Lighting/LightData.h>

// data
#include <Data/Engine/Configs/Scene/Objects/Material/MaterialConfig.h>

/* c++ */
#include<stdint.h>
#include<string>

//===================================================================*/
//			meshまてりある 
//===================================================================*/
struct Material{

	Vector4 color = { 1, 1, 1, 1 };;
	int32_t lightingMode = LightingMode::HalfLambert;
	float pad[3];
	Matrix4x4 uvTransform;
	float shininess;
	bool isReflect = false;
	float enviromentCoefficient = 0.5f;
	float roughness = 0.5f; // 反射率

	//config
	void ApplyConfig(const MaterialConfig& config);
	MaterialConfig ExtractConfig() const;
	void ShowImGui();
	void ShowImGui(MaterialConfig& config);


};

//===================================================================*/
//				パーティクルmaterial
//===================================================================*/
struct ParticleMaterial{
	Vector4 color = {1, 1, 1, 1};  // 基本色（tint）
	Matrix4x4 uvTransform = Matrix4x4::MakeIdentity(); // UVアニメ用
	std::string texturePath = "particle.png"; // テクスチャパス
};

//===================================================================*/
//				2dmaterial
//===================================================================*/
struct Material2D{
	Vector4 color;
	Matrix4x4 uvTransform;
};

//===================================================================*/
//				gpu転送用データ
//===================================================================*/
struct MaterialData{
	std::string textureFilePath;
	Vector3 uv_scale;
	Vector3 uv_offset;
	Vector3 uv_translate;
};