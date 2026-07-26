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

/*-----------------------------------------------------------------------------------------
 * Material
 * - Object3D描画用のGPU定数とMaterial設定を保持するデータ構造
 * - Lighting、UV、Toon、Emissive、Normal Map、Rim Light設定を管理する
 * - Texture ResourceとDescriptorの所有は担当しない
 *---------------------------------------------------------------------------------------*/
struct Material {

	CalyxEngine::Vector4   color        = {1,1,1,1};;
	int32_t   lightingMode = LightingMode::HalfLambert;
	float     pad[3];
	CalyxEngine::Matrix4x4 uvTransform;
	float     shininess;
	int32_t   isReflect             = 0;
	float     envirometCoefficient = 0.5f;
	float     roughness             = 0.5f; // 反射率
	CalyxEngine::Vector4 toonHighlightColor = {1.15f, 1.10f, 1.00f, 1.0f};
	CalyxEngine::Vector4 toonBaseColor = {1.0f, 1.0f, 1.0f, 1.0f};
	CalyxEngine::Vector4 toonMidShadowColor = {0.72f, 0.76f, 0.86f, 1.0f};
	CalyxEngine::Vector4 toonShadowColor = {0.42f, 0.46f, 0.58f, 1.0f};
	float     toonBaseStep = 0.25f;
	float     toonBaseFeather = 0.03f;
	float     toonShadeStep = -0.15f;
	float     toonShadeFeather = 0.03f;
	float     toonSpecularThreshold = 0.96f;
	float     toonSpecularSoftness = 0.02f;
	float     toonSpecularIntensity = 0.35f;
	float     pad3 = 0.0f;
	CalyxEngine::Vector4 emissiveColor = {0.0f, 0.0f, 0.0f, 1.0f};
	float     emissiveIntensity = 0.0f;
	int32_t   useNormalMap = 0;
	float     normalMapStrength = 1.0f;
	int32_t   normalMapFlipY = 0;
	CalyxEngine::Vector4 rimColor = {1.0f, 1.0f, 1.0f, 1.0f};
	float rimIntensity = 0.0f;
	float rimPower = 3.0f;
	float rimPadding[2] = {};

	//config
	void           ApplyConfig(const MaterialConfig& config);
	MaterialConfig ExtractConfig() const;
	void           ShowImGui();
	void           ShowImGui(MaterialConfig& config);


};

/*-----------------------------------------------------------------------------------------
 * ParticleMaterial
 * - Particle描画へ渡すMaterial定数とTexture参照情報を保持するデータ構造
 * - UV Animation、Noise Mask、Runtime経過時間をGPU配置に合わせて管理する
 *---------------------------------------------------------------------------------------*/
struct ParticleMaterial {
	CalyxEngine::Vector4     color       = {1,1,1,1};     // 基本色（tint）
	// xy: 固定Offset, zw: Tiling。GPU Constant Bufferの16-byte境界を維持する。
	CalyxEngine::Vector4     uvOffsetTiling = {0.0f,0.0f,1.0f,1.0f};
	// xy: Scroll Speed, z: Rotation(radian), w: Runtime elapsed time。
	CalyxEngine::Vector4     uvScrollRotationTime = {0.0f,0.0f,0.0f,0.0f};
	// x: mask texture attached, y: scale, z: strength, w: threshold
	CalyxEngine::Vector4     noiseMaskParams = {0.0f,1.0f,1.0f,0.5f};
	// xy: runtime UV offset, z: edge softness, w: padding
	CalyxEngine::Vector4     noiseMaskUv = {0.0f,0.0f,0.1f,0.0f};
	std::string texturePath = "Textures/white1x1.dds";  // テクスチャパス
};

/*-----------------------------------------------------------------------------------------
 * Material2D
 * - Spriteなど2D描画用のMaterial定数を保持するデータ構造
 * - 色、UV変換、Fill表現とGPUアラインメント用領域を管理する
 *---------------------------------------------------------------------------------------*/
struct Material2D {
	CalyxEngine::Vector4   color;
	CalyxEngine::Matrix4x4 uvTransform;

	CalyxEngine::Vector2 uvOffset;
	CalyxEngine::Vector2 uvScale = {1.0f,1.0f};

	float   fillAmount = 1.0f;        // 0〜1 (初期は1、全部描画)
	CalyxEngine::Vector2 fillOrigin = {0.0f,0.0f}; // (0,0)=左/下, (1,0)=右/下, etc.
	int     fillMethod = 0;           // 0=none, 1=horizontal, 2=vertical, 3=mask

	float padding; // 16バイト境界を合わせる
};

/*-----------------------------------------------------------------------------------------
 * MaterialData
 * - Model Assetから読み込んだMaterial参照情報を保持するデータ構造
 * - TextureパスとUV変換値をCPU側の中間データとして管理する
 *---------------------------------------------------------------------------------------*/
struct MaterialData {
	std::string textureFilePath;
	std::string normalTextureFilePath;
	CalyxEngine::Vector3     uv_scale;
	CalyxEngine::Vector3     uv_offset;
	CalyxEngine::Vector3     uv_translate;
};
