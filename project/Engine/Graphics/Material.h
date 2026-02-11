#pragma once

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

struct Material {

	CalyxMath::Vector4   color        = {1,1,1,1};;
	int32_t   lightingMode = LightingMode::HalfLambert;
	float     pad[3];
	CalyxMath::Matrix4x4 uvTransform;
	float     shininess;
	bool      isReflect             = false;
	float     envirometCoefficient = 0.5f;
	float     roughness             = 0.5f; // 反射率

	//config
	void           ApplyConfig(const MaterialConfig& config);
	MaterialConfig ExtractConfig() const;
	void           ShowImGui();
	void           ShowImGui(MaterialConfig& config);


};

struct ParticleMaterial {
	CalyxMath::Vector4     color       = {1,1,1,1};     // 基本色（tint）
	CalyxMath::Matrix4x4   uvTransform = CalyxMath::Matrix4x4::MakeIdentity(); // UVアニメ用
	std::string texturePath = "particle.dds";            // テクスチャパス
};

struct Material2D {
	CalyxMath::Vector4   color;
	CalyxMath::Matrix4x4 uvTransform;

	CalyxMath::Vector2 uvOffset;
	CalyxMath::Vector2 uvScale = {1.0f,1.0f};

	float   fillAmount = 1.0f;        // 0〜1 (初期は1、全部描画)
	CalyxMath::Vector2 fillOrigin = {0.0f,0.0f}; // (0,0)=左/下, (1,0)=右/下, etc.
	int     fillMethod = 0;           // 0=none, 1=horizontal, 2=vertical, 3=mask

	float padding; // 16バイト境界を合わせる
};

struct MaterialData {
	std::string textureFilePath;
	CalyxMath::Vector3     uv_scale;
	CalyxMath::Vector3     uv_offset;
	CalyxMath::Vector3     uv_translate;
};