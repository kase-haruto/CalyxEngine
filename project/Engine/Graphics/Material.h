#pragma once

// math
#include <Engine/Foundation/Math/Mat3.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

// data
#include <Data/Engine/Configs/Scene/Objects/Material/MaterialConfig.h>

/* c++ */
#include<stdint.h>
#include<string>

struct Material{
	Vector4 color;
	int32_t enableLighting;
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

struct ParticleMaterial{
	Vector4 color = {1, 1, 1, 1};  // 基本色（tint）
	Matrix4x4 uvTransform = Matrix4x4::MakeIdentity(); // UVアニメ用
	std::string texturePath = "particle.png"; // テクスチャパス
};

struct Material2D{
	Vector4 color;
	Matrix4x4 uvTransform;
};

struct MaterialData{
	std::string textureFilePath;
	Vector3 uv_scale;
	Vector3 uv_offset;
	Vector3 uv_translate;
};