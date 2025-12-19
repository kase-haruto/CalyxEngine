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

	CxMath::Vector4   color        = {1,1,1,1};;
	int32_t   lightingMode = LightingMode::HalfLambert;
	float     pad[3];
	CxMath::Matrix4x4 uvTransform;
	float     shininess;
	bool      isReflect             = false;
	float     enviromentCoefficient = 0.5f;
	float     roughness             = 0.5f; // 反射率

	//config
	void           ApplyConfig(const MaterialConfig& config);
	MaterialConfig ExtractConfig() const;
	void           ShowImGui();
	void           ShowImGui(MaterialConfig& config);


};

struct ParticleMaterial {
	CxMath::Vector4     color       = {1,1,1,1};     // 基本色（tint）
	CxMath::Matrix4x4   uvTransform = CxMath::Matrix4x4::MakeIdentity(); // UVアニメ用
	std::string texturePath = "particle.png";            // テクスチャパス
};

struct Material2D {
	CxMath::Vector4   color;
	CxMath::Matrix4x4 uvTransform;

	float   fillAmount = 1.0f;        // 0〜1 (初期は1、全部描画)
	Vector2 fillOrigin = {0.0f,0.0f}; // (0,0)=左/下, (1,0)=右/下, etc.
	int     fillMethod = 0;           // 0=none, 1=horizontal, 2=vertical, 3=mask

	float padding; // 16バイト境界を合わせる
};

struct MaterialData {
	std::string textureFilePath;
	CxMath::Vector3     uv_scale;
	CxMath::Vector3     uv_offset;
	CxMath::Vector3     uv_translate;
};