#pragma once
/////////////////////////////////////////////////////////////////////////////////////////
//	include space
/////////////////////////////////////////////////////////////////////////////////////////

// engine
#include <Engine/Graphics/MaterialGraph/ShaderGraphExpressionCompiler.h>

// c++
#include <sstream>
#include <string>

namespace CalyxEngine {
	/////////////////////////////////////////////////////////////////////////////////////////
	//		生成済みShader Graphコード
	/////////////////////////////////////////////////////////////////////////////////////////
	struct GeneratedShaderGraphCode {
		std::string hlsl;
		bool usesObjectTexture = false;
		int32_t textureSlotCount = 0;
	};

	/////////////////////////////////////////////////////////////////////////////////////////
	//		式IRをHLSLソースへ変換するEmitter
	/////////////////////////////////////////////////////////////////////////////////////////
	class ShaderGraphHlslEmitter {
	public:
		/////////////////////////////////////////////////////////////////////////////////////////
		//		式IRからマテリアル評価関数を生成
		/////////////////////////////////////////////////////////////////////////////////////////
		static GeneratedShaderGraphCode GenerateMaterialFunction(const ShaderGraphExpressionIR& surface) {
			GeneratedShaderGraphCode code;

			// 生成後のバインド処理で必要になるテクスチャ利用情報を集約する。
			code.usesObjectTexture =
				surface.baseColor.usesObjectTexture ||
				surface.emissiveColor.usesObjectTexture ||
				surface.normalMap.usesObjectTexture ||
				surface.toonHighlightColor.usesObjectTexture ||
				surface.toonBaseColor.usesObjectTexture ||
				surface.toonFirstShadeColor.usesObjectTexture ||
				surface.toonSecondShadeColor.usesObjectTexture;
			code.textureSlotCount = surface.textureSlotCount;

			// Noise関数を必要な場合だけ出力するため、全Surface式を確認する。
			const bool usesNoiseTexture =
				surface.baseColor.usesNoiseTexture ||
				surface.emissiveColor.usesNoiseTexture ||
				surface.emissiveIntensity.usesNoiseTexture ||
				surface.shininess.usesNoiseTexture ||
				surface.roughness.usesNoiseTexture ||
				surface.normalMap.usesNoiseTexture ||
				surface.normalMapStrength.usesNoiseTexture ||
				surface.toonHighlightColor.usesNoiseTexture ||
				surface.toonBaseColor.usesNoiseTexture ||
				surface.toonFirstShadeColor.usesNoiseTexture ||
				surface.toonSecondShadeColor.usesNoiseTexture ||
				surface.toonBaseStep.usesNoiseTexture ||
				surface.toonBaseFeather.usesNoiseTexture ||
				surface.toonShadeStep.usesNoiseTexture ||
				surface.toonShadeFeather.usesNoiseTexture ||
				surface.toonSpecularThreshold.usesNoiseTexture ||
				surface.toonSpecularSoftness.usesNoiseTexture ||
				surface.toonSpecularIntensity.usesNoiseTexture;


			// マテリアル評価結果を返すHLSL構造体を宣言する。
			std::ostringstream out;
			out << "struct GeneratedMaterialSurface {\n";
			out << "    int lightingMode;\n";
			out << "    float4 baseColor;\n";
			out << "    float4 emissiveColor;\n";
			out << "    float emissiveIntensity;\n";
			out << "    float shininess;\n";
			out << "    float roughness;\n";
			out << "    int isReflect;\n";
			out << "    float4 normalMap;\n";
			out << "    float normalMapStrength;\n";
			out << "    float4 toonHighlightColor;\n";
			out << "    float4 toonBaseColor;\n";
			out << "    float4 toonFirstShadeColor;\n";
			out << "    float4 toonSecondShadeColor;\n";
			out << "    float toonBaseStep;\n";
			out << "    float toonBaseFeather;\n";
			out << "    float toonShadeStep;\n";
			out << "    float toonShadeFeather;\n";
			out << "    float toonSpecularThreshold;\n";
			out << "    float toonSpecularSoftness;\n";
			out << "    float toonSpecularIntensity;\n";
			out << "};\n\n";
			if(usesNoiseTexture) {
				// Noiseノードを使用するShaderだけへ補助関数を追加する。
				AppendNoiseTextureFunctions(out);
			}

			// IRが保持する各HLSL式をSurface評価関数へ書き出す。
			out << "GeneratedMaterialSurface EvaluateGeneratedMaterial(float2 uv, float3 worldPosition, float3 normal, float3 viewDirection) {\n";
			out << "    GeneratedMaterialSurface surface;\n";
			out << "    surface.lightingMode = " << surface.lightingMode << ";\n";
			out << "    surface.baseColor = " << surface.baseColor.hlsl << ";\n";
			out << "    surface.emissiveColor = " << surface.emissiveColor.hlsl << ";\n";
			out << "    surface.emissiveIntensity = " << surface.emissiveIntensity.hlsl << ";\n";
			out << "    surface.shininess = " << surface.shininess.hlsl << ";\n";
			out << "    surface.roughness = " << surface.roughness.hlsl << ";\n";
			out << "    surface.isReflect = " << (surface.isReflect ? 1 : 0) << ";\n";
			out << "    surface.normalMap = " << surface.normalMap.hlsl << ";\n";
			out << "    surface.normalMapStrength = " << surface.normalMapStrength.hlsl << ";\n";
			out << "    surface.toonHighlightColor = " << surface.toonHighlightColor.hlsl << ";\n";
			out << "    surface.toonBaseColor = " << surface.toonBaseColor.hlsl << ";\n";
			out << "    surface.toonFirstShadeColor = " << surface.toonFirstShadeColor.hlsl << ";\n";
			out << "    surface.toonSecondShadeColor = " << surface.toonSecondShadeColor.hlsl << ";\n";
			out << "    surface.toonBaseStep = " << surface.toonBaseStep.hlsl << ";\n";
			out << "    surface.toonBaseFeather = " << surface.toonBaseFeather.hlsl << ";\n";
			out << "    surface.toonShadeStep = " << surface.toonShadeStep.hlsl << ";\n";
			out << "    surface.toonShadeFeather = " << surface.toonShadeFeather.hlsl << ";\n";
			out << "    surface.toonSpecularThreshold = " << surface.toonSpecularThreshold.hlsl << ";\n";
			out << "    surface.toonSpecularSoftness = " << surface.toonSpecularSoftness.hlsl << ";\n";
			out << "    surface.toonSpecularIntensity = " << surface.toonSpecularIntensity.hlsl << ";\n";
			out << "    return surface;\n";
			out << "}\n";

			code.hlsl = out.str();
			return code;
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		プレビュー用Pixel Shaderを生成
		/////////////////////////////////////////////////////////////////////////////////////////
		static GeneratedShaderGraphCode GeneratePreviewPixelShader(const GeneratedShaderGraphCode& materialFunction) {
			GeneratedShaderGraphCode code;

			// マテリアル関数が収集したリソース情報を完成Shaderへ引き継ぐ。
			code.usesObjectTexture = materialFunction.usesObjectTexture;
			code.textureSlotCount = materialFunction.textureSlotCount;


			// プレビュー描画で使用する最小限のリソースと入出力を宣言する。
			std::ostringstream out;
			out << "Texture2D<float4> gTexture : register(t0);\n";
			out << "Texture2D<float4> gGraphTextures[8] : register(t9);\n";
			out << "SamplerState gSampler : register(s0);\n\n";
			out << "struct Material {\n";
			out << "    float4 color;\n";
			out << "    int enableLighting;\n";
			out << "    float3 pad;\n";
			out << "    float4x4 uvTransform;\n";
			out << "    float shiniess;\n";
			out << "    int isReflect;\n";
			out << "    float environmentCoefficient;\n";
			out << "    float roughness;\n";
			out << "    float4 toonHighlightColor;\n";
			out << "    float4 toonBaseColor;\n";
			out << "    float4 toonMidShadowColor;\n";
			out << "    float4 toonShadowColor;\n";
			out << "    float toonBaseStep;\n";
			out << "    float toonBaseFeather;\n";
			out << "    float toonShadeStep;\n";
			out << "    float toonShadeFeather;\n";
			out << "    float toonSpecularThreshold;\n";
			out << "    float toonSpecularSoftness;\n";
			out << "    float toonSpecularIntensity;\n";
			out << "    float pad3;\n";
			out << "    float4 emissiveColor;\n";
			out << "    float emissiveIntensity;\n";
			out << "    int useNormalMap;\n";
			out << "    float normalMapStrength;\n";
			out << "    int normalMapFlipY;\n";
			out << "    float4 rimColor;\n";
			out << "    float rimIntensity;\n";
			out << "    float rimPower;\n";
			out << "    float2 rimPadding;\n";
			out << "    float4 shieldColor;\n";
			out << "    float4 shieldParams;\n";
			out << "    float4 shieldRipple;\n";
			out << "};\n";
			out << "cbuffer MaterialConstants : register(b0) { Material gMaterial; }\n\n";
			out << "struct VertexShaderOutput {\n";
			out << "    float4 position : SV_POSITION;\n";
			out << "    float2 texcoord : TEXCOORD0;\n";
			out << "};\n\n";
			out << "struct PixelShaderOutput {\n";
			out << "    float4 color : SV_TARGET0;\n";
			out << "};\n\n";
			// 生成済みマテリアル評価関数をプレビューShaderへ組み込む。
			out << materialFunction.hlsl << "\n";
			out << "PixelShaderOutput main(VertexShaderOutput input) {\n";
			out << "    PixelShaderOutput output;\n";
			out << "    GeneratedMaterialSurface surface = EvaluateGeneratedMaterial(input.texcoord, float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 1.0f), float3(0.0f, 0.0f, 1.0f));\n";
			out << "    output.color = float4(saturate(surface.baseColor.rgb + surface.emissiveColor.rgb * max(surface.emissiveIntensity, 0.0f)), surface.baseColor.a);\n";
			out << "    return output;\n";
			out << "}\n";

			code.hlsl = out.str();
			return code;
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		Object3Dランタイム用Pixel Shaderを生成
		/////////////////////////////////////////////////////////////////////////////////////////
		static GeneratedShaderGraphCode GenerateRuntimePixelShader(const GeneratedShaderGraphCode& materialFunction) {
			GeneratedShaderGraphCode code;

			// マテリアル関数が収集したリソース情報を完成Shaderへ引き継ぐ。
			code.usesObjectTexture = materialFunction.usesObjectTexture;
			code.textureSlotCount = materialFunction.textureSlotCount;


			// Object3D描画で使用する頂点出力と定数バッファを宣言する。
			std::ostringstream out;
			out << "struct VertexShaderOutput {\n";
			out << "    float4 position : SV_POSITION;\n";
			out << "    float2 texcoord : TEXCOORD0;\n";
			out << "    float3 normal : NORMAL0;\n";
			out << "    float3 worldPosition : POSITION0;\n";
			out << "    float4 tangent : TANGENT0;\n";
			out << "};\n\n";
			out << "cbuffer CameraConstants : register(b1) {\n";
			out << "    float4x4 View;\n";
			out << "    float4x4 Projection;\n";
			out << "    float4x4 ViewProjection;\n";
			out << "    float3 cameraPosition;\n";
			out << "    float3 camRight;\n";
			out << "    float3 camUp;\n";
			out << "    float3 camForward;\n";
			out << "    float2 viewportSize;\n";
			out << "    uint cameraDitherEnabled;\n";
			out << "}\n\n";
			out << "cbuffer ObjectDrawConstants : register(b6) { uint objectDitherEnabled; }\n\n";
			out << "struct Material {\n";
			out << "    float4 color;\n";
			out << "    int enableLighting;\n";
			out << "    float3 pad;\n";
			out << "    float4x4 uvTransform;\n";
			out << "    float shiniess;\n";
			out << "    int isReflect;\n";
			out << "    float environmentCoefficient;\n";
			out << "    float roughness;\n";
			out << "    float4 toonHighlightColor;\n";
			out << "    float4 toonBaseColor;\n";
			out << "    float4 toonMidShadowColor;\n";
			out << "    float4 toonShadowColor;\n";
			out << "    float toonBaseStep;\n";
			out << "    float toonBaseFeather;\n";
			out << "    float toonShadeStep;\n";
			out << "    float toonShadeFeather;\n";
			out << "    float toonSpecularThreshold;\n";
			out << "    float toonSpecularSoftness;\n";
			out << "    float toonSpecularIntensity;\n";
			out << "    float pad3;\n";
			out << "    float4 emissiveColor;\n";
			out << "    float emissiveIntensity;\n";
			out << "    int useNormalMap;\n";
			out << "    float normalMapStrength;\n";
			out << "    int normalMapFlipY;\n";
			out << "    float4 rimColor;\n";
			out << "    float rimIntensity;\n";
			out << "    float rimPower;\n";
			out << "    float2 rimPadding;\n";
			out << "    float4 shieldColor;\n";
			out << "    float4 shieldParams;\n";
			out << "    float4 shieldRipple;\n";
			out << "};\n\n";
			out << "struct DirectionalLight { float4 color; float3 direction; float intensity; };\n";
			out << "struct PointLight { float4 color; float3 position; float intensity; float radius; float decay; float2 pad; };\n\n";
			out << "cbuffer MaterialConstants : register(b0) { Material gMaterial; }\n";
			out << "cbuffer DirectionalLightConstants : register(b2) { DirectionalLight gDirectionalLight; }\n";
			out << "cbuffer ShadowConstants : register(b3) { float4x4 gLightVP; float gShadowBias; float3 _shadowPad; };\n";
			out << "cbuffer PointLightConstants : register(b4) { uint gPointLightCount; uint gPointLightShadowsEnabled; uint gMaxPointShadowLights; float gPointShadowContributionThreshold; PointLight gPointLights[16]; }\n";
			out << "cbuffer RaytracingShadowParamConstants : register(b5) { float gShadowRayEps; float gBaseAngularRadius; float gMinShadow; bool gIsSoft; };\n\n";
			out << "Texture2D<float4> gTexture : register(t0);\n";
			out << "TextureCube<float4> gEnvironmentMap : register(t1);\n";
			out << "Texture2D<float> gShadowMap : register(t2);\n";
			out << "RaytracingAccelerationStructure gRtScene : register(t3);\n";
			out << "Texture2D<float4> gNormalMap : register(t4);\n";
			out << "Texture2D<float4> gGraphTextures[8] : register(t9);\n";
			out << "SamplerState gSampler : register(s0);\n\n";
			out << "struct PixelShaderOutput {\n";
			out << "    float4 color : SV_TARGET0;\n";
			out << "    float4 bloomMask : SV_TARGET1;\n";
			out << "};\n\n";
			// 生成済みマテリアル評価関数をランタイムShaderへ組み込む。
			out << materialFunction.hlsl << "\n";

			// Object3D共通のライティング、Shadow、Normal Map処理を追加する。
			out << R"(

///////////////////////////////////////////////////////////////////////////////
//                    繝・ぅ繧ｶ繝槭ャ繝・
///////////////////////////////////////////////////////////////////////////////
static const float4x4 kBayerMatrix = float4x4(
	0.0 / 16.0, 8.0 / 16.0, 2.0 / 16.0, 10.0 / 16.0,
	12.0 / 16.0, 4.0 / 16.0, 14.0 / 16.0, 6.0 / 16.0,
	3.0 / 16.0, 11.0 / 16.0, 1.0 / 16.0, 9.0 / 16.0,
	15.0 / 16.0, 7.0 / 16.0, 13.0 / 16.0, 5.0 / 16.0
);

float3 ApplyToneMappingAndGamma(float3 color, float exposure) {
    float3 toneMapped = color * exposure / (color * exposure + 1.0f);
    return pow(toneMapped, 1.0 / 2.2);
}

float ToonBandGenerated(float value, float threshold, float softness) {
    float width = max(softness, 0.0001f);
    return smoothstep(threshold - width, threshold + width, value);
}

float3 EvaluateGeneratedToonRamp(float ndotl, float3 albedo, GeneratedMaterialSurface surface) {
    float shadeStep = min(surface.toonShadeStep, surface.toonBaseStep);
    float baseStep = max(surface.toonShadeStep, surface.toonBaseStep);
    float3 shadow = albedo * surface.toonSecondShadeColor.rgb;
    float3 midShadow = albedo * surface.toonFirstShadeColor.rgb;
    float3 base = albedo * surface.toonBaseColor.rgb;
    float3 shadeRamp = lerp(shadow, midShadow, ToonBandGenerated(ndotl, shadeStep, surface.toonShadeFeather));
    return lerp(shadeRamp, base, ToonBandGenerated(ndotl, baseStep, surface.toonBaseFeather));
}

float EvaluateGeneratedToonSpecular(float ndoth, GeneratedMaterialSurface surface) {
    return ToonBandGenerated(ndoth, saturate(surface.toonSpecularThreshold), surface.toonSpecularSoftness) *
           max(surface.toonSpecularIntensity, 0.0f);
}

void ComputeGeneratedStandardDirectionalLight(float3 normal, float3 toEye, float3 albedo, GeneratedMaterialSurface surface, out float3 diffuse, out float3 specular) {
    diffuse = 0.0f;
    specular = 0.0f;
    float3 L = -gDirectionalLight.direction;
    float rawNdotL = dot(normal, L);
    float NdotL = saturate(rawNdotL);
    float3 H = normalize(L + toEye);
    float NdotH = saturate(dot(normal, H));
    float roughness = saturate(surface.roughness);
    float specularWeight = pow(1.0f - roughness, 2.0f);
    if(surface.lightingMode == 0) {
        float halfLambert = pow(rawNdotL * 0.5f + 0.5f, 2.0f);
        diffuse = albedo * gDirectionalLight.color.rgb * halfLambert * gDirectionalLight.intensity;
        specular = gDirectionalLight.color.rgb * pow(NdotH, surface.shininess) * gDirectionalLight.intensity * specularWeight;
    } else if(surface.lightingMode == 1) {
        diffuse = albedo * gDirectionalLight.color.rgb * NdotL * gDirectionalLight.intensity;
        specular = gDirectionalLight.color.rgb * pow(NdotH, surface.shininess) * gDirectionalLight.intensity * specularWeight;
    }
}

bool CheckVisibility(float3 origin, float3 dir, float tMax);
float ComputePointHardShadow_RT(float3 worldPos, float3 normal, float3 lightPos, float lightDistance);

void ComputeGeneratedStandardPointLight(float3 normal, float3 toEye, float3 worldPos, float3 albedo, GeneratedMaterialSurface surface, out float3 diffuse, out float3 specular) {
    diffuse = 0.0f;
    specular = 0.0f;
    float topContribution0 = 0.0f;
    float topContribution1 = 0.0f;
    float3 topDiffuse0 = 0.0f;
    float3 topDiffuse1 = 0.0f;
    float3 topSpecular0 = 0.0f;
    float3 topSpecular1 = 0.0f;
    float3 topPosition0 = 0.0f;
    float3 topPosition1 = 0.0f;
    float topDistance0 = 0.0f;
    float topDistance1 = 0.0f;
    float roughness = saturate(surface.roughness);
    float specularWeight = pow(1.0f - roughness, 2.0f);
    [loop]
    for(uint i = 0; i < gPointLightCount; ++i) {
        PointLight pointLight = gPointLights[i];
        if(pointLight.intensity <= 0.0f || pointLight.radius <= 0.0f) {
            continue;
        }
        float3 lightDir = normalize(worldPos - pointLight.position);
        float distance = length(pointLight.position - worldPos);
        float attenuation = pow(saturate(1.0f - distance / pointLight.radius), pointLight.decay);
        float NdotL = saturate(dot(normal, -lightDir));
        float contribution = NdotL * pointLight.intensity * attenuation;
        float3 lightDiffuse = albedo * pointLight.color.rgb * contribution;
        diffuse += lightDiffuse;
        float3 halfVec = normalize(-lightDir + toEye);
        float NdotH = saturate(dot(normal, halfVec));
        float3 lightSpecular = pointLight.color.rgb * pow(NdotH, surface.shininess) * pointLight.intensity * attenuation * specularWeight;
        specular += lightSpecular;
        if(gPointLightShadowsEnabled != 0 && contribution > gPointShadowContributionThreshold) {
            if(contribution > topContribution0) {
                topContribution1 = topContribution0;
                topDiffuse1 = topDiffuse0;
                topSpecular1 = topSpecular0;
                topPosition1 = topPosition0;
                topDistance1 = topDistance0;
                topContribution0 = contribution;
                topDiffuse0 = lightDiffuse;
                topSpecular0 = lightSpecular;
                topPosition0 = pointLight.position;
                topDistance0 = distance;
            } else if(contribution > topContribution1) {
                topContribution1 = contribution;
                topDiffuse1 = lightDiffuse;
                topSpecular1 = lightSpecular;
                topPosition1 = pointLight.position;
                topDistance1 = distance;
            }
        }
    }
    if(gPointLightShadowsEnabled != 0 && gMaxPointShadowLights > 0 && topContribution0 > 0.0f) {
        float shadow0 = ComputePointHardShadow_RT(worldPos, normal, topPosition0, topDistance0);
        diffuse += topDiffuse0 * (shadow0 - 1.0f);
        specular += topSpecular0 * (shadow0 - 1.0f);
    }
    if(gPointLightShadowsEnabled != 0 && gMaxPointShadowLights > 1 && topContribution1 > 0.0f) {
        float shadow1 = ComputePointHardShadow_RT(worldPos, normal, topPosition1, topDistance1);
        diffuse += topDiffuse1 * (shadow1 - 1.0f);
        specular += topSpecular1 * (shadow1 - 1.0f);
    }
}

void ComputeGeneratedToonDirectionalLight(float3 normal, float3 toEye, float3 albedo, GeneratedMaterialSurface surface, out float3 diffuse, out float3 specular) {
    float3 L = -gDirectionalLight.direction;
    float rawNdotL = dot(normal, L);
    diffuse = EvaluateGeneratedToonRamp(rawNdotL, albedo, surface) * gDirectionalLight.color.rgb * gDirectionalLight.intensity;
    float3 H = normalize(L + toEye);
    float NdotH = saturate(dot(normal, H));
    float toonSpecular = EvaluateGeneratedToonSpecular(NdotH, surface);
    specular = gDirectionalLight.color.rgb * surface.toonHighlightColor.rgb * toonSpecular * gDirectionalLight.intensity;
}

void ComputeGeneratedToonPointLight(float3 normal, float3 toEye, float3 worldPos, float3 albedo, GeneratedMaterialSurface surface, out float3 diffuse, out float3 specular) {
    diffuse = 0.0f;
    specular = 0.0f;
    float topContribution0 = 0.0f;
    float topContribution1 = 0.0f;
    float3 topDiffuse0 = 0.0f;
    float3 topDiffuse1 = 0.0f;
    float3 topSpecular0 = 0.0f;
    float3 topSpecular1 = 0.0f;
    float3 topPosition0 = 0.0f;
    float3 topPosition1 = 0.0f;
    float topDistance0 = 0.0f;
    float topDistance1 = 0.0f;
    [loop]
    for(uint i = 0; i < gPointLightCount; ++i) {
        PointLight pointLight = gPointLights[i];
        if(pointLight.intensity <= 0.0f || pointLight.radius <= 0.0f) {
            continue;
        }
        float3 lightDir = normalize(worldPos - pointLight.position);
        float distance = length(pointLight.position - worldPos);
        float attenuation = pow(saturate(1.0f - distance / pointLight.radius), pointLight.decay);
        float rawNdotL = dot(normal, -lightDir);
        float contribution = saturate(rawNdotL) * pointLight.intensity * attenuation;
        float3 lightDiffuse = EvaluateGeneratedToonRamp(rawNdotL, albedo, surface) * pointLight.color.rgb * pointLight.intensity * attenuation;
        diffuse += lightDiffuse;
        float3 halfVec = normalize(-lightDir + toEye);
        float NdotH = saturate(dot(normal, halfVec));
        float toonSpecular = EvaluateGeneratedToonSpecular(NdotH, surface);
        float3 lightSpecular = pointLight.color.rgb * surface.toonHighlightColor.rgb * toonSpecular * pointLight.intensity * attenuation;
        specular += lightSpecular;
        if(gPointLightShadowsEnabled != 0 && contribution > gPointShadowContributionThreshold) {
            if(contribution > topContribution0) {
                topContribution1 = topContribution0;
                topDiffuse1 = topDiffuse0;
                topSpecular1 = topSpecular0;
                topPosition1 = topPosition0;
                topDistance1 = topDistance0;
                topContribution0 = contribution;
                topDiffuse0 = lightDiffuse;
                topSpecular0 = lightSpecular;
                topPosition0 = pointLight.position;
                topDistance0 = distance;
            } else if(contribution > topContribution1) {
                topContribution1 = contribution;
                topDiffuse1 = lightDiffuse;
                topSpecular1 = lightSpecular;
                topPosition1 = pointLight.position;
                topDistance1 = distance;
            }
        }
    }
    if(gPointLightShadowsEnabled != 0 && gMaxPointShadowLights > 0 && topContribution0 > 0.0f) {
        float shadow0 = ComputePointHardShadow_RT(worldPos, normal, topPosition0, topDistance0);
        diffuse += topDiffuse0 * (shadow0 - 1.0f);
        specular += topSpecular0 * (shadow0 - 1.0f);
    }
    if(gPointLightShadowsEnabled != 0 && gMaxPointShadowLights > 1 && topContribution1 > 0.0f) {
        float shadow1 = ComputePointHardShadow_RT(worldPos, normal, topPosition1, topDistance1);
        diffuse += topDiffuse1 * (shadow1 - 1.0f);
        specular += topSpecular1 * (shadow1 - 1.0f);
    }
}

float Hash21(float2 p) {
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

float2 Rotate2D(float2 v, float a) {
    float s, c;
    sincos(a, s, c);
    return float2(c * v.x - s * v.y, s * v.x + c * v.y);
}

void BuildOrthonormalBasis(float3 n, out float3 t, out float3 b) {
    float3 up = (abs(n.z) < 0.999f) ? float3(0,0,1) : float3(0,1,0);
    t = normalize(cross(up, n));
    b = cross(n, t);
}

static const float2 kPoisson16[16] = {
    float2(-0.326f, -0.406f), float2(-0.840f, -0.074f), float2(-0.696f,  0.457f), float2(-0.203f,  0.621f),
    float2( 0.962f, -0.195f), float2( 0.473f, -0.480f), float2( 0.519f,  0.767f), float2( 0.185f, -0.893f),
    float2(-0.433f,  0.043f), float2( 0.267f,  0.271f), float2(-0.123f, -0.754f), float2( 0.735f,  0.128f),
    float2( 0.078f,  0.912f), float2(-0.917f,  0.381f), float2( 0.621f, -0.727f), float2(-0.589f, -0.201f)
};

bool CheckVisibility(float3 origin, float3 dir, float tMax) {
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = 0.0f;
    ray.TMax = tMax;
    RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
    q.TraceRayInline(gRtScene, RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, ray);
    q.Proceed();
    return (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT);
}

float ComputeDirectionalHardShadow_RT(float3 worldPos, float3 normal, float3 L) {
    float3 origin = worldPos + normal * gShadowRayEps;
    return CheckVisibility(origin, L, 1000.0f) ? gMinShadow : 1.0f;
}

float ComputePointHardShadow_RT(float3 worldPos, float3 normal, float3 lightPos, float lightDistance) {
    float3 origin = worldPos + normal * gShadowRayEps;
    float3 toLight = lightPos - origin;
    float tMax = min(max(length(toLight) - gShadowRayEps, 0.0f), lightDistance);
    if(tMax <= 0.0f) {
        return 1.0f;
    }
    float3 L = normalize(toLight);
    return CheckVisibility(origin, L, tMax) ? gMinShadow : 1.0f;
}

float ComputeDirectionalSoftShadow_RT(float3 worldPos, float3 normal, float3 L) {
    if(!gIsSoft) return 1.0f;
    float3 origin = worldPos + normal * gShadowRayEps;
    const float tMax = 1000.0f;
    bool centerHit = CheckVisibility(origin, L, tMax);
    if(!centerHit) return 1.0f;
    float angularRadius = max(gBaseAngularRadius, 0.0f);
    int sampleCount = 16;
    if(angularRadius < 0.010f) sampleCount = 4;
    else if(angularRadius < 0.025f) sampleCount = 8;
    float ndotl = saturate(dot(normal, L));
    if(ndotl < 0.35f) sampleCount = min(sampleCount, 4);
    else if(ndotl < 0.60f) sampleCount = min(sampleCount, 8);
    int extraCount = max(sampleCount - 1, 0);
    float3 T, B;
    BuildOrthonormalBasis(L, T, B);
    float rnd = Hash21(worldPos.xz * 17.0f + worldPos.yy * 3.0f);
    float ang = rnd * 6.2831853f;
    float occluded = 1.0f;
    const float kDarkEarlyOut = 0.75f;
    [loop]
    for(int i = 0; i < extraCount; ++i) {
        float2 d = Rotate2D(kPoisson16[i], ang) * angularRadius;
        float3 dirJ = normalize(L + T * d.x + B * d.y);
        bool hit = CheckVisibility(origin, dirJ, tMax);
        occluded += hit ? 1.0f : 0.0f;
        if(occluded >= (float)sampleCount * kDarkEarlyOut) return gMinShadow;
    }
    float shadow = 1.0f - (occluded / (float)sampleCount);
    return lerp(gMinShadow, 1.0f, shadow);
}
)";
			out << R"(

float3 ApplyGeneratedNormalMap(VertexShaderOutput input, float3 vertexNormal, GeneratedMaterialSurface surface) {
    if(surface.normalMapStrength <= 0.0f) {
        return vertexNormal;
    }

    float3 tangent = normalize(input.tangent.xyz);
    tangent = normalize(tangent - vertexNormal * dot(tangent, vertexNormal));
    float3 bitangent = normalize(cross(vertexNormal, tangent) * input.tangent.w);

    float3 normalTexel = surface.normalMap.rgb;
    if(all(normalTexel > 0.99f)) {
        return vertexNormal;
    }

    float grayscaleDelta = max(abs(normalTexel.r - normalTexel.g), abs(normalTexel.g - normalTexel.b));
    if(grayscaleDelta < 0.03f) {
        float height = normalTexel.r;
        float heightDx = ddx(height);
        float heightDy = ddy(height);
        float2 uvDx = ddx(input.texcoord);
        float2 uvDy = ddy(input.texcoord);
        float texelScale = max(max(length(uvDx), length(uvDy)), 0.0001f);
        float2 heightSlope = float2(heightDx, heightDy) / texelScale;
        float3 bumpNormal = normalize(float3(-heightSlope.x, -heightSlope.y, 1.0f));
        bumpNormal.xy *= max(surface.normalMapStrength, 0.0f);
        bumpNormal = normalize(bumpNormal);
        float3x3 bumpTbn = float3x3(tangent, bitangent, vertexNormal);
        return normalize(mul(bumpNormal, bumpTbn));
    }

    float3 sampledNormal = normalTexel * 2.0f - 1.0f;
    if(gMaterial.normalMapFlipY != 0) {
        sampledNormal.y *= -1.0f;
    }
    sampledNormal.xy *= max(surface.normalMapStrength, 0.0f);
    sampledNormal = normalize(sampledNormal);

    float3x3 tbn = float3x3(tangent, bitangent, vertexNormal);
    return normalize(mul(sampledNormal, tbn));
}

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float3 toEye = normalize(cameraPosition - input.worldPosition);
    float3 normal = normalize(input.normal);
    GeneratedMaterialSurface surface = EvaluateGeneratedMaterial(transformedUV.xy, input.worldPosition, normal, toEye);
    normal = ApplyGeneratedNormalMap(input, normal, surface);
    float3 albedo = surface.baseColor.rgb;
    float alpha = surface.baseColor.a;
    float3 emissive = surface.emissiveColor.rgb * max(surface.emissiveIntensity, 0.0f);

    if(surface.lightingMode == 4) {
        if(alpha <= 0.01f) discard;
        output.color = float4(ApplyToneMappingAndGamma(albedo, 1.0f) + emissive, alpha);
        output.bloomMask = float4(emissive, 1.0f);
        return output;
    }

    float3 directionalDiffuse = 0.0f;
    float3 directionalSpecular = 0.0f;
    float3 pointDiffuse = 0.0f;
    float3 pointSpecular = 0.0f;

    if(surface.lightingMode == 2) {
        ComputeGeneratedToonDirectionalLight(normal, toEye, albedo, surface, directionalDiffuse, directionalSpecular);
        ComputeGeneratedToonPointLight(normal, toEye, input.worldPosition, albedo, surface, pointDiffuse, pointSpecular);
    } else {
        ComputeGeneratedStandardDirectionalLight(normal, toEye, albedo, surface, directionalDiffuse, directionalSpecular);
        ComputeGeneratedStandardPointLight(normal, toEye, input.worldPosition, albedo, surface, pointDiffuse, pointSpecular);
    }

    float3 L = normalize(-gDirectionalLight.direction);
    float shadow = 1.0f;
    if(dot(normal, L) > 0.0f) {
        shadow = gIsSoft ? ComputeDirectionalSoftShadow_RT(input.worldPosition, normal, L) : ComputeDirectionalHardShadow_RT(input.worldPosition, normal, L);
    }
    directionalDiffuse *= shadow;
    directionalSpecular *= shadow;

    float3 litColor = directionalDiffuse + directionalSpecular + pointDiffuse + pointSpecular;
    float rimFactor = pow(saturate(1.0f - dot(normal, toEye)), max(gMaterial.rimPower, 0.0001f));
    litColor += gMaterial.rimColor.rgb * rimFactor * max(gMaterial.rimIntensity, 0.0f);
    litColor += emissive;
    litColor += albedo * 0.07f;

    if(surface.isReflect != 0) {
        float3 viewDir = normalize(input.worldPosition - cameraPosition);
        float3 reflectDir = reflect(viewDir, normal);
        const float maxMipLevel = 7.0f;
        float roughness = saturate(surface.roughness);
        float mipLevel = roughness * maxMipLevel;
        float reflectionWeight = gMaterial.environmentCoefficient * pow(1.0f - roughness, 2.0f);
        float3 envColor = gEnvironmentMap.SampleLevel(gSampler, reflectDir, mipLevel).rgb;
        litColor += envColor * reflectionWeight;
    }

    float3 finalColor = ApplyToneMappingAndGamma(litColor, 1.0f);

	// Dithered clipping
	uint2 pixelPos = uint2(input.position.xy) % 4;
	float ditherThreshold = kBayerMatrix[pixelPos.y][pixelPos.x];

	float dist = length(input.worldPosition - cameraPosition);
	float fadeNear = 2.5f;
	float fadeFar = 10.0f;
	float fade = saturate((dist - fadeNear) / (fadeFar - fadeNear));

	if(cameraDitherEnabled != 0 && objectDitherEnabled != 0 && fade <= ditherThreshold) {
		discard;
	}

if(alpha <= 0.01f) discard;
    output.color = float4(finalColor, alpha);
    float3 rimEmission = gMaterial.rimColor.rgb * rimFactor * max(gMaterial.rimIntensity, 0.0f);
    output.bloomMask = float4(emissive + rimEmission, 1.0f);
    return output;
}
)";

			code.hlsl = out.str();
			return code;
		}

	private:
		/////////////////////////////////////////////////////////////////////////////////////////
		//		Noise Texture用HLSL関数を追加
		/////////////////////////////////////////////////////////////////////////////////////////
		static void AppendNoiseTextureFunctions(std::ostringstream& out) {
			out << R"(
float GeneratedNoiseHash21(float2 p) {
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

float GeneratedValueNoise(float2 p) {
    float2 i = floor(p);
    float2 f = frac(p);
    float a = GeneratedNoiseHash21(i);
    float b = GeneratedNoiseHash21(i + float2(1.0f, 0.0f));
    float c = GeneratedNoiseHash21(i + float2(0.0f, 1.0f));
    float d = GeneratedNoiseHash21(i + float2(1.0f, 1.0f));
    float2 u = f * f * (3.0f - 2.0f * f);
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

)";
		}
	};
}
