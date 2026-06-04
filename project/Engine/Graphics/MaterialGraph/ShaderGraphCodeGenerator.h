#pragma once

#include <Engine/Assets/DataAsset/MaterialAsset.h>
#include <Engine/Graphics/MaterialGraph/CompiledMaterialGraph.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace CalyxEngine {
	struct GeneratedShaderGraphCode {
		std::string hlsl;
		bool usesObjectTexture = false;
		int32_t textureSlotCount = 0;
	};

	class ShaderGraphCodeGenerator {
	public:
		static constexpr int32_t kMaxGraphTextures = 8;

		static GeneratedShaderGraphCode GenerateObject3DMaterialFunction(const MaterialAsset& material) {
			MaterialExpressionSurface surface = CompileMaterialExpressions(material);
			return GenerateObject3DMaterialFunction(surface);
		}

		static GeneratedShaderGraphCode GeneratePreviewPixelShaderSource(const MaterialAsset& material) {
			GeneratedShaderGraphCode materialFunction = GenerateObject3DMaterialFunction(CompileMaterialExpressions(material, false));
			return GeneratePreviewPixelShaderSource(materialFunction);
		}

		static GeneratedShaderGraphCode GenerateObject3DRuntimePixelShaderSource(const MaterialAsset& material) {
			GeneratedShaderGraphCode materialFunction = GenerateObject3DMaterialFunction(CompileMaterialExpressions(material, true));
			return GenerateObject3DRuntimePixelShaderSource(materialFunction);
		}

		static GeneratedShaderGraphCode GenerateObject3DMaterialFunction(const CompiledMaterialGraph& graph) {
			GeneratedShaderGraphCode code;
			code.usesObjectTexture =
				graph.baseColor.usesObjectTexture ||
				graph.emissiveColor.usesObjectTexture ||
				graph.normalMap.usesObjectTexture ||
				graph.toonHighlightColor.usesObjectTexture ||
				graph.toonBaseColor.usesObjectTexture ||
				graph.toonFirstShadeColor.usesObjectTexture ||
				graph.toonSecondShadeColor.usesObjectTexture;

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

			out << "GeneratedMaterialSurface EvaluateGeneratedMaterial(float2 uv, float3 worldPosition, float3 normal, float3 viewDirection) {\n";
			out << "    GeneratedMaterialSurface surface;\n";
			if(code.usesObjectTexture) {
				out << "    float4 objectTexture = gTexture.Sample(gSampler, uv);\n";
			}
			out << "    surface.lightingMode = " << graph.lightingMode << ";\n";
			out << "    surface.baseColor = " << ColorExpr(graph.baseColor) << ";\n";
			out << "    surface.emissiveColor = " << ColorExpr(graph.emissiveColor) << ";\n";
			out << "    surface.emissiveIntensity = " << FloatExpr(graph.emissiveIntensity) << ";\n";
			out << "    surface.shininess = " << FloatExpr(graph.shininess) << ";\n";
			out << "    surface.roughness = " << FloatExpr(graph.roughness) << ";\n";
			out << "    surface.isReflect = " << (graph.isReflect ? 1 : 0) << ";\n";
			out << "    surface.normalMap = " << ColorExpr(graph.normalMap) << ";\n";
			out << "    surface.normalMapStrength = " << FloatExpr(graph.normalMapStrength) << ";\n";
			out << "    surface.toonHighlightColor = " << ColorExpr(graph.toonHighlightColor) << ";\n";
			out << "    surface.toonBaseColor = " << ColorExpr(graph.toonBaseColor) << ";\n";
			out << "    surface.toonFirstShadeColor = " << ColorExpr(graph.toonFirstShadeColor) << ";\n";
			out << "    surface.toonSecondShadeColor = " << ColorExpr(graph.toonSecondShadeColor) << ";\n";
			out << "    surface.toonBaseStep = " << FloatExpr(graph.toonBaseStep) << ";\n";
			out << "    surface.toonBaseFeather = " << FloatExpr(graph.toonBaseFeather) << ";\n";
			out << "    surface.toonShadeStep = " << FloatExpr(graph.toonShadeStep) << ";\n";
			out << "    surface.toonShadeFeather = " << FloatExpr(graph.toonShadeFeather) << ";\n";
			out << "    surface.toonSpecularThreshold = " << FloatExpr(graph.toonSpecularThreshold) << ";\n";
			out << "    surface.toonSpecularSoftness = " << FloatExpr(graph.toonSpecularSoftness) << ";\n";
			out << "    surface.toonSpecularIntensity = " << FloatExpr(graph.toonSpecularIntensity) << ";\n";
			out << "    return surface;\n";
			out << "}\n";

			code.hlsl = out.str();
			return code;
		}

		static GeneratedShaderGraphCode GeneratePreviewPixelShaderSource(const CompiledMaterialGraph& graph) {
			GeneratedShaderGraphCode materialFunction = GenerateObject3DMaterialFunction(graph);
			return GeneratePreviewPixelShaderSource(materialFunction);
		}

	private:
		struct MaterialExpression {
			std::string hlsl;
			bool usesObjectTexture = false;
			bool usesNoiseTexture = false;
		};

		struct MaterialExpressionSurface {
			int32_t lightingMode = 0;
			int32_t textureSlotCount = 0;
			MaterialExpression baseColor;
			MaterialExpression emissiveColor;
			MaterialExpression emissiveIntensity;
			MaterialExpression shininess;
			MaterialExpression roughness;
			bool isReflect = false;
			MaterialExpression normalMap;
			MaterialExpression normalMapStrength;
			MaterialExpression toonHighlightColor;
			MaterialExpression toonBaseColor;
			MaterialExpression toonFirstShadeColor;
			MaterialExpression toonSecondShadeColor;
			MaterialExpression toonBaseStep;
			MaterialExpression toonBaseFeather;
			MaterialExpression toonShadeStep;
			MaterialExpression toonShadeFeather;
			MaterialExpression toonSpecularThreshold;
			MaterialExpression toonSpecularSoftness;
			MaterialExpression toonSpecularIntensity;
		};

		static GeneratedShaderGraphCode GenerateObject3DMaterialFunction(const MaterialExpressionSurface& surface) {
			GeneratedShaderGraphCode code;
			code.usesObjectTexture =
				surface.baseColor.usesObjectTexture ||
				surface.emissiveColor.usesObjectTexture ||
				surface.normalMap.usesObjectTexture ||
				surface.toonHighlightColor.usesObjectTexture ||
				surface.toonBaseColor.usesObjectTexture ||
				surface.toonFirstShadeColor.usesObjectTexture ||
				surface.toonSecondShadeColor.usesObjectTexture;
			code.textureSlotCount = surface.textureSlotCount;
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
				AppendNoiseTextureFunctions(out);
			}

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

		static GeneratedShaderGraphCode GeneratePreviewPixelShaderSource(const GeneratedShaderGraphCode& materialFunction) {
			GeneratedShaderGraphCode code;
			code.usesObjectTexture = materialFunction.usesObjectTexture;
			code.textureSlotCount = materialFunction.textureSlotCount;

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
			out << "};\n";
			out << "cbuffer MaterialConstants : register(b0) { Material gMaterial; }\n\n";
			out << "struct VertexShaderOutput {\n";
			out << "    float4 position : SV_POSITION;\n";
			out << "    float2 texcoord : TEXCOORD0;\n";
			out << "};\n\n";
			out << "struct PixelShaderOutput {\n";
			out << "    float4 color : SV_TARGET0;\n";
			out << "};\n\n";
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

		static GeneratedShaderGraphCode GenerateObject3DRuntimePixelShaderSource(const GeneratedShaderGraphCode& materialFunction) {
			GeneratedShaderGraphCode code;
			code.usesObjectTexture = materialFunction.usesObjectTexture;
			code.textureSlotCount = materialFunction.textureSlotCount;

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
			out << materialFunction.hlsl << "\n";
			out << R"(

///////////////////////////////////////////////////////////////////////////////
//                    ディザマップ
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
    output.bloomMask = float4(emissive, 1.0f);
    return output;
}
)";

			code.hlsl = out.str();
			return code;
		}

		static std::string FloatExpr(float value) {
			std::ostringstream out;
			out << std::fixed << std::setprecision(6) << value << "f";
			return out.str();
		}

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

		static std::string Float4Expr(const Vector4& value) {
			return "float4(" +
				   FloatExpr(value.x) + ", " +
				   FloatExpr(value.y) + ", " +
				   FloatExpr(value.z) + ", " +
				   FloatExpr(value.w) + ")";
		}

		static std::string ColorExpr(const CompiledColorInput& input) {
			const std::string factor = Float4Expr(input.factor);
			if(!input.usesObjectTexture) return factor;
			return "(" + factor + " * objectTexture)";
		}

		static MaterialExpression FloatExpression(float value) {
			return {FloatExpr(value), false};
		}

		static MaterialExpression ColorExpression(const Vector4& value) {
			return {Float4Expr(value), false};
		}

		static MaterialExpression Combine(std::string hlsl, const MaterialExpression& a, const MaterialExpression& b) {
			return {std::move(hlsl), a.usesObjectTexture || b.usesObjectTexture, a.usesNoiseTexture || b.usesNoiseTexture};
		}

		static MaterialExpression NoiseValueExpression(const NodeGraph& graph, const Node& node, int32_t depth) {
			MaterialExpression noiseUv = {"uv", false};
			if(const NodePin* uvPin = FindInput(node, "UV")) {
				noiseUv = Float2ExpressionFromInput(graph, uvPin->id, noiseUv, depth + 1);
			}
			MaterialExpression scale = FloatExpression(GetFloatProperty(node, "scale", 8.0f));
			if(const NodePin* scalePin = FindInput(node, "Scale")) {
				scale = FloatExpressionFromInput(graph, scalePin->id, scale, depth + 1);
			}
			return {
				"GeneratedValueNoise(" + noiseUv.hlsl + " * max(" + scale.hlsl + ", 0.0001f))",
				noiseUv.usesObjectTexture || scale.usesObjectTexture,
				true};
		}

		static const Node* FindOutput(const NodeGraph& graph) {
			for(const Node& node : graph.nodes) {
				if(node.type == "Output") return &node;
			}
			return nullptr;
		}

		static const NodePin* FindInput(const Node& node, const std::string& name) {
			for(const NodePin& pin : node.inputs) {
				if(pin.name == name) return &pin;
			}
			return nullptr;
		}

		static const Node* FindLinkedNode(const NodeGraph& graph, int32_t inputPinId) {
			for(const NodeLink& link : graph.links) {
				if(link.toPinId != inputPinId) continue;
				const Node* fromNode = nullptr;
				graph.FindPin(link.fromPinId, &fromNode);
				return fromNode;
			}
			return nullptr;
		}

		static int32_t TextureSlotCount(const NodeGraph& graph) {
			int32_t count = 0;
			for(const Node& node : graph.nodes) {
				if(IsTextureSourceNode(node)) ++count;
			}
			return count;
		}

		static bool IsTextureSourceNode(const Node& node) {
			return node.type == "ObjectTexture" || node.type == "Texture2D";
		}

		static int32_t TextureSlotForTextureSourceNode(const NodeGraph& graph, const Node& textureNode) {
			int32_t slot = 0;
			for(const Node& node : graph.nodes) {
				if(!IsTextureSourceNode(node)) continue;
				if(node.id == textureNode.id) return slot;
				++slot;
			}
			return 0;
		}

		static int32_t TextureSlotForSampleNode(const NodeGraph& graph, const Node& sampleNode) {
			const NodePin* texturePin = FindInput(sampleNode, "Texture");
			const Node* textureNode = texturePin ? FindLinkedNode(graph, texturePin->id) : nullptr;
			if(!textureNode || !IsTextureSourceNode(*textureNode)) return 0;
			return (std::min)(TextureSlotForTextureSourceNode(graph, *textureNode), kMaxGraphTextures - 1);
		}

		static bool HasInputLink(const NodeGraph& graph, int32_t inputPinId) {
			return std::any_of(graph.links.begin(), graph.links.end(), [inputPinId](const NodeLink& link) {
				return link.toPinId == inputPinId;
			});
		}

		static float GetFloatProperty(const Node& node, const char* key, float fallback) {
			if(!node.properties.contains(key)) return fallback;
			return node.properties.value(key, fallback);
		}

		static Vector4 GetColorProperty(const Node& node, const char* key, const Vector4& fallback) {
			auto it = node.properties.find(key);
			if(it == node.properties.end() || !it->is_array() || it->size() != 4) return fallback;
			return {it->at(0).get<float>(), it->at(1).get<float>(), it->at(2).get<float>(), it->at(3).get<float>()};
		}

		static MaterialExpression ColorInputExpression(const MaterialAsset& material, const Node& node, const char* inputName, const Vector4& fallback) {
			return ColorInputExpression(material, node, inputName, ColorExpression(fallback));
		}

		static MaterialExpression ColorInputExpression(const MaterialAsset& material, const Node& node, const char* inputName, const MaterialExpression& fallback) {
			const NodePin* pin = FindInput(node, inputName);
			if(!pin || !HasInputLink(material.graph, pin->id)) return fallback;
			return ColorExpressionFromInput(material.graph, pin->id, fallback, 0);
		}

		static MaterialExpression FloatInputExpression(const MaterialAsset& material, const Node& node, const char* inputName, float fallback) {
			return FloatInputExpression(material, node, inputName, FloatExpression(fallback));
		}

		static MaterialExpression FloatInputExpression(const MaterialAsset& material, const Node& node, const char* inputName, const MaterialExpression& fallback) {
			const NodePin* pin = FindInput(node, inputName);
			if(!pin || !HasInputLink(material.graph, pin->id)) return fallback;
			return FloatExpressionFromInput(material.graph, pin->id, fallback, 0);
		}

		static MaterialExpression Float2Expression(float x, float y) {
			return {"float2(" + FloatExpr(x) + ", " + FloatExpr(y) + ")", false};
		}

		static MaterialExpression Float2ExpressionFromInput(
			const NodeGraph& graph,
			int32_t inputPinId,
			const MaterialExpression& fallback,
			int32_t depth) {
			if(depth > 32) return fallback;
			const Node* fromNode = FindLinkedNode(graph, inputPinId);
			if(!fromNode) return fallback;

			if(fromNode->type == "UV") return {"uv", false};
			if(fromNode->type == "Float2") {
				return Float2Expression(
					GetFloatProperty(*fromNode, "x", 0.0f),
					GetFloatProperty(*fromNode, "y", 0.0f));
			}
			if(fromNode->type == "UVTransform") {
				MaterialExpression sourceUv = {"uv", false};
				MaterialExpression scale = Float2Expression(
					GetFloatProperty(*fromNode, "scaleX", 1.0f),
					GetFloatProperty(*fromNode, "scaleY", 1.0f));
				MaterialExpression offset = Float2Expression(
					GetFloatProperty(*fromNode, "offsetX", 0.0f),
					GetFloatProperty(*fromNode, "offsetY", 0.0f));
				if(const NodePin* uvPin = FindInput(*fromNode, "UV")) {
					sourceUv = Float2ExpressionFromInput(graph, uvPin->id, sourceUv, depth + 1);
				}
				if(const NodePin* scalePin = FindInput(*fromNode, "Scale")) {
					scale = Float2ExpressionFromInput(graph, scalePin->id, scale, depth + 1);
				}
				if(const NodePin* offsetPin = FindInput(*fromNode, "Offset")) {
					offset = Float2ExpressionFromInput(graph, offsetPin->id, offset, depth + 1);
				}
				return {
					"((" + sourceUv.hlsl + " * " + scale.hlsl + ") + " + offset.hlsl + ")",
					sourceUv.usesObjectTexture || scale.usesObjectTexture || offset.usesObjectTexture,
					sourceUv.usesNoiseTexture || scale.usesNoiseTexture || offset.usesNoiseTexture};
			}
			if(fromNode->type == "CombineFloat2" && fromNode->inputs.size() >= 2) {
				const MaterialExpression x = FloatExpressionFromInput(graph, fromNode->inputs[0].id, FloatExpression(0.0f), depth + 1);
				const MaterialExpression y = FloatExpressionFromInput(graph, fromNode->inputs[1].id, FloatExpression(0.0f), depth + 1);
				return {"float2(" + x.hlsl + ", " + y.hlsl + ")", x.usesObjectTexture || y.usesObjectTexture, x.usesNoiseTexture || y.usesNoiseTexture};
			}
			if(fromNode->type == "AddFloat2" && fromNode->inputs.size() >= 2) {
				const MaterialExpression a = Float2ExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const MaterialExpression b = Float2ExpressionFromInput(graph, fromNode->inputs[1].id, {"float2(0.0f, 0.0f)", false}, depth + 1);
				return Combine("(" + a.hlsl + " + " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "SubtractFloat2" && fromNode->inputs.size() >= 2) {
				const MaterialExpression a = Float2ExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const MaterialExpression b = Float2ExpressionFromInput(graph, fromNode->inputs[1].id, {"float2(0.0f, 0.0f)", false}, depth + 1);
				return Combine("(" + a.hlsl + " - " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "MultiplyFloat2" && fromNode->inputs.size() >= 2) {
				const MaterialExpression a = Float2ExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const MaterialExpression b = Float2ExpressionFromInput(graph, fromNode->inputs[1].id, {"float2(1.0f, 1.0f)", false}, depth + 1);
				return Combine("(" + a.hlsl + " * " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "DivideFloat2" && fromNode->inputs.size() >= 2) {
				const MaterialExpression a = Float2ExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const MaterialExpression b = Float2ExpressionFromInput(graph, fromNode->inputs[1].id, {"float2(1.0f, 1.0f)", false}, depth + 1);
				return Combine("(" + a.hlsl + " / max(abs(" + b.hlsl + "), float2(0.0001f, 0.0001f)))", a, b);
			}
			return fallback;
		}

		static MaterialExpression ColorExpressionFromInput(
			const NodeGraph& graph,
			int32_t inputPinId,
			const MaterialExpression& fallback,
			int32_t depth) {
			if(depth > 32) return fallback;
			const Node* fromNode = FindLinkedNode(graph, inputPinId);
			if(!fromNode) return fallback;

			if(fromNode->type == "Color") return ColorExpression(fromNode->colorValue);
			if(fromNode->type == "TextureSample") {
				MaterialExpression sampleUv = {"uv", false};
				if(const NodePin* uvPin = FindInput(*fromNode, "UV")) {
					sampleUv = Float2ExpressionFromInput(graph, uvPin->id, sampleUv, depth + 1);
				}
				const int32_t textureSlot = TextureSlotForSampleNode(graph, *fromNode);
				return {"gGraphTextures[" + std::to_string(textureSlot) + "].Sample(gSampler, " + sampleUv.hlsl + ")", true};
			}
			if(fromNode->type == "NoiseTexture") {
				const MaterialExpression value = NoiseValueExpression(graph, *fromNode, depth);
				return {"float4(" + value.hlsl + ", " + value.hlsl + ", " + value.hlsl + ", 1.0f)", value.usesObjectTexture, value.usesNoiseTexture};
			}
			if(fromNode->type == "MultiplyColor" && fromNode->inputs.size() >= 2) {
				const MaterialExpression a = ColorExpressionFromInput(graph, fromNode->inputs[0].id, ColorExpression({1, 1, 1, 1}), depth + 1);
				const MaterialExpression b = ColorExpressionFromInput(graph, fromNode->inputs[1].id, ColorExpression({1, 1, 1, 1}), depth + 1);
				return Combine("(" + a.hlsl + " * " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "LerpColor" && fromNode->inputs.size() >= 3) {
				const MaterialExpression a = ColorExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const MaterialExpression b = ColorExpressionFromInput(graph, fromNode->inputs[1].id, fallback, depth + 1);
				const MaterialExpression t = FloatExpressionFromInput(graph, fromNode->inputs[2].id, FloatExpression(0.0f), depth + 1);
				return {
					("lerp(" + a.hlsl + ", " + b.hlsl + ", saturate(" + t.hlsl + "))"),
					a.usesObjectTexture || b.usesObjectTexture || t.usesObjectTexture,
					a.usesNoiseTexture || b.usesNoiseTexture || t.usesNoiseTexture};
			}
			return fallback;
		}

		static MaterialExpression FloatExpressionFromInput(
			const NodeGraph& graph,
			int32_t inputPinId,
			const MaterialExpression& fallback,
			int32_t depth) {
			if(depth > 32) return fallback;
			const Node* fromNode = FindLinkedNode(graph, inputPinId);
			if(!fromNode) return fallback;

			const NodePin* fromPin = nullptr;
			for(const NodeLink& link : graph.links) {
				if(link.toPinId == inputPinId) {
					fromPin = graph.FindPin(link.fromPinId);
					break;
				}
			}

			if(fromNode->type == "Float" || fromNode->type == "Shininess" || fromNode->type == "Roughness") return FloatExpression(fromNode->floatValue);
			if(fromNode->type == "TextureSample") {
				MaterialExpression sampleUv = {"uv", false};
				if(const NodePin* uvPin = FindInput(*fromNode, "UV")) {
					sampleUv = Float2ExpressionFromInput(graph, uvPin->id, sampleUv, depth + 1);
				}
				const int32_t textureSlot = TextureSlotForSampleNode(graph, *fromNode);
				const std::string sample = "gGraphTextures[" + std::to_string(textureSlot) + "].Sample(gSampler, " + sampleUv.hlsl + ")";
				if(fromPin && fromPin->name == "Value") {
					return {"dot((" + sample + ").rgb, float3(0.299f, 0.587f, 0.114f))", true};
				}
				return fallback;
			}
			if(fromNode->type == "NoiseTexture") return NoiseValueExpression(graph, *fromNode, depth);
			if(fromNode->type == "SplitFloat2" && fromPin && !fromNode->inputs.empty()) {
				const MaterialExpression value = Float2ExpressionFromInput(graph, fromNode->inputs[0].id, {"float2(0.0f, 0.0f)", false}, depth + 1);
				if(fromPin->name == "X") return {value.hlsl + ".x", value.usesObjectTexture, value.usesNoiseTexture};
				if(fromPin->name == "Y") return {value.hlsl + ".y", value.usesObjectTexture, value.usesNoiseTexture};
			}
			if(fromNode->type == "UVX") return {"uv.x", false};
			if(fromNode->type == "UVY") return {"uv.y", false};
			if(fromNode->type == "Time") return {"gMaterial.pad3", false};
			if(fromNode->type == "WorldPositionX") return {"worldPosition.x", false};
			if(fromNode->type == "WorldPositionY") return {"worldPosition.y", false};
			if(fromNode->type == "WorldPositionZ") return {"worldPosition.z", false};
			if(fromNode->type == "WorldNormalX") return {"normal.x", false};
			if(fromNode->type == "WorldNormalY") return {"normal.y", false};
			if(fromNode->type == "WorldNormalZ") return {"normal.z", false};
			if(fromNode->type == "ViewDirectionX") return {"viewDirection.x", false};
			if(fromNode->type == "ViewDirectionY") return {"viewDirection.y", false};
			if(fromNode->type == "ViewDirectionZ") return {"viewDirection.z", false};
			if(fromNode->type == "AddFloat" && fromNode->inputs.size() >= 2) {
				const MaterialExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, FloatExpression(0.0f), depth + 1);
				const MaterialExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, FloatExpression(0.0f), depth + 1);
				return Combine("(" + a.hlsl + " + " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "SubtractFloat" && fromNode->inputs.size() >= 2) {
				const MaterialExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, FloatExpression(0.0f), depth + 1);
				const MaterialExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, FloatExpression(0.0f), depth + 1);
				return Combine("(" + a.hlsl + " - " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "MultiplyFloat" && fromNode->inputs.size() >= 2) {
				const MaterialExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, FloatExpression(1.0f), depth + 1);
				const MaterialExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, FloatExpression(1.0f), depth + 1);
				return Combine("(" + a.hlsl + " * " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "DivideFloat" && fromNode->inputs.size() >= 2) {
				const MaterialExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const MaterialExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, FloatExpression(1.0f), depth + 1);
				return Combine("(" + a.hlsl + " / ((" + b.hlsl + ") < 0.0f ? -max(abs(" + b.hlsl + "), 0.0001f) : max(abs(" + b.hlsl + "), 0.0001f)))", a, b);
			}
			if(fromNode->type == "PowerFloat" && fromNode->inputs.size() >= 2) {
				const MaterialExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, FloatExpression(1.0f), depth + 1);
				const MaterialExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, FloatExpression(1.0f), depth + 1);
				return Combine("pow(max(" + a.hlsl + ", 0.0f), " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "MinFloat" && fromNode->inputs.size() >= 2) {
				const MaterialExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const MaterialExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, fallback, depth + 1);
				return Combine("min(" + a.hlsl + ", " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "MaxFloat" && fromNode->inputs.size() >= 2) {
				const MaterialExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const MaterialExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, fallback, depth + 1);
				return Combine("max(" + a.hlsl + ", " + b.hlsl + ")", a, b);
			}
			if(fromNode->type == "LerpFloat" && fromNode->inputs.size() >= 3) {
				const MaterialExpression a = FloatExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				const MaterialExpression b = FloatExpressionFromInput(graph, fromNode->inputs[1].id, fallback, depth + 1);
				const MaterialExpression t = FloatExpressionFromInput(graph, fromNode->inputs[2].id, FloatExpression(0.0f), depth + 1);
				return {
					("lerp(" + a.hlsl + ", " + b.hlsl + ", saturate(" + t.hlsl + "))"),
					a.usesObjectTexture || b.usesObjectTexture || t.usesObjectTexture,
					a.usesNoiseTexture || b.usesNoiseTexture || t.usesNoiseTexture};
			}
			if(fromNode->type == "SaturateFloat" && !fromNode->inputs.empty()) {
				const MaterialExpression value = FloatExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				return {"saturate(" + value.hlsl + ")", value.usesObjectTexture, value.usesNoiseTexture};
			}
			if(fromNode->type == "FracFloat" && !fromNode->inputs.empty()) {
				const MaterialExpression value = FloatExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				return {"frac(" + value.hlsl + ")", value.usesObjectTexture, value.usesNoiseTexture};
			}
			if(fromNode->type == "OneMinusFloat" && !fromNode->inputs.empty()) {
				const MaterialExpression value = FloatExpressionFromInput(graph, fromNode->inputs[0].id, fallback, depth + 1);
				return {"(1.0f - " + value.hlsl + ")", value.usesObjectTexture, value.usesNoiseTexture};
			}
			if(fromNode->type == "StepFloat" && fromNode->inputs.size() >= 2) {
				const MaterialExpression edge = FloatExpressionFromInput(graph, fromNode->inputs[0].id, FloatExpression(0.5f), depth + 1);
				const MaterialExpression value = FloatExpressionFromInput(graph, fromNode->inputs[1].id, fallback, depth + 1);
				return {
					"step(" + edge.hlsl + ", " + value.hlsl + ")",
					edge.usesObjectTexture || value.usesObjectTexture,
					edge.usesNoiseTexture || value.usesNoiseTexture};
			}
			if(fromNode->type == "SmoothstepFloat" && fromNode->inputs.size() >= 3) {
				const MaterialExpression edge0 = FloatExpressionFromInput(graph, fromNode->inputs[0].id, FloatExpression(0.4f), depth + 1);
				const MaterialExpression edge1 = FloatExpressionFromInput(graph, fromNode->inputs[1].id, FloatExpression(0.6f), depth + 1);
				const MaterialExpression value = FloatExpressionFromInput(graph, fromNode->inputs[2].id, fallback, depth + 1);
				return {
					"smoothstep(" + edge0.hlsl + ", " + edge1.hlsl + ", " + value.hlsl + ")",
					edge0.usesObjectTexture || edge1.usesObjectTexture || value.usesObjectTexture,
					edge0.usesNoiseTexture || edge1.usesNoiseTexture || value.usesNoiseTexture};
			}
			return fallback;
		}

		static MaterialExpressionSurface MakeDefaultExpressionSurface(const MaterialAsset& material, bool useMaterialCBufferDefaults) {
			MaterialExpressionSurface surface;
			surface.textureSlotCount = (std::min)(TextureSlotCount(material.graph), kMaxGraphTextures);
			surface.lightingMode = material.lightingMode;
			surface.baseColor = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.color", false} : ColorExpression(material.color);
			surface.emissiveColor = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.emissiveColor", false} : ColorExpression(material.emissiveColor);
			surface.emissiveIntensity = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.emissiveIntensity", false} : FloatExpression(material.emissiveIntensity);
			surface.shininess = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.shiniess", false} : FloatExpression(material.shininess);
			surface.roughness = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.roughness", false} : FloatExpression(material.roughness);
			surface.isReflect = material.isReflect;
			surface.normalMap = useMaterialCBufferDefaults
				? MaterialExpression{"(gMaterial.useNormalMap != 0 ? gNormalMap.Sample(gSampler, uv) : float4(0.5f, 0.5f, 1.0f, 1.0f))", false}
				: ColorExpression({0.5f, 0.5f, 1.0f, 1.0f});
			surface.normalMapStrength = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.normalMapStrength", false} : FloatExpression(material.normalMapStrength);
			surface.toonHighlightColor = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.toonHighlightColor", false} : ColorExpression(material.toonHighlightColor);
			surface.toonBaseColor = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.toonBaseColor", false} : ColorExpression(material.toonBaseColor);
			surface.toonFirstShadeColor = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.toonMidShadowColor", false} : ColorExpression(material.toonMidShadowColor);
			surface.toonSecondShadeColor = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.toonShadowColor", false} : ColorExpression(material.toonShadowColor);
			surface.toonBaseStep = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.toonBaseStep", false} : FloatExpression(material.toonBaseStep);
			surface.toonBaseFeather = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.toonBaseFeather", false} : FloatExpression(material.toonBaseFeather);
			surface.toonShadeStep = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.toonShadeStep", false} : FloatExpression(material.toonShadeStep);
			surface.toonShadeFeather = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.toonShadeFeather", false} : FloatExpression(material.toonShadeFeather);
			surface.toonSpecularThreshold = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.toonSpecularThreshold", false} : FloatExpression(material.toonSpecularThreshold);
			surface.toonSpecularSoftness = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.toonSpecularSoftness", false} : FloatExpression(material.toonSpecularSoftness);
			surface.toonSpecularIntensity = useMaterialCBufferDefaults ? MaterialExpression{"gMaterial.toonSpecularIntensity", false} : FloatExpression(material.toonSpecularIntensity);
			return surface;
		}

		static void ApplyLegacyOutputExpressions(const MaterialAsset& material, const Node& output, MaterialExpressionSurface& surface) {
			for(const NodePin& pin : output.inputs) {
				if(pin.name == "BaseColor") surface.baseColor = ColorExpressionFromInput(material.graph, pin.id, surface.baseColor, 0);
				if(pin.name == "Emissive") surface.emissiveColor = ColorExpressionFromInput(material.graph, pin.id, surface.emissiveColor, 0);
				if(pin.name == "Emissive Intensity") surface.emissiveIntensity = FloatExpressionFromInput(material.graph, pin.id, surface.emissiveIntensity, 0);
				if(pin.name == "Shininess") surface.shininess = FloatExpressionFromInput(material.graph, pin.id, surface.shininess, 0);
				if(pin.name == "Roughness") surface.roughness = FloatExpressionFromInput(material.graph, pin.id, surface.roughness, 0);
				if(pin.name == "Lighting Mode") {
					if(const Node* linked = FindLinkedNode(material.graph, pin.id)) {
						if(linked->type == "LambertLighting") surface.lightingMode = 1;
						else if(linked->type == "ToonLighting") surface.lightingMode = 2;
						else if(linked->type == "NoLighting") surface.lightingMode = 3;
						else if(linked->type == "UnlitColorLighting") surface.lightingMode = 4;
						else surface.lightingMode = 0;
					}
				}
			}
		}

		static MaterialExpressionSurface CompileMaterialExpressions(const MaterialAsset& material, bool useMaterialCBufferDefaults = false) {
			MaterialExpressionSurface surface = MakeDefaultExpressionSurface(material, useMaterialCBufferDefaults);
			const Node* output = FindOutput(material.graph);
			if(!output) return surface;

			const NodePin* surfacePin = FindInput(*output, "Surface");
			const Node* master = surfacePin ? FindLinkedNode(material.graph, surfacePin->id) : nullptr;
			if(!master) {
				ApplyLegacyOutputExpressions(material, *output, surface);
				return surface;
			}

			if(master->type == "ToonMaster") {
				surface.lightingMode = 2;
				const Vector4 baseColor = GetColorProperty(*master, "baseColor", {1, 1, 1, 1});
				const Vector4 emissiveColor = GetColorProperty(*master, "emissiveColor", material.emissiveColor);
				const Vector4 highlightColor = GetColorProperty(*master, "highlightColor", {1.08f, 1.06f, 1.02f, 1.0f});
				const Vector4 firstShade = GetColorProperty(*master, "firstShadeColor", {0.72f, 0.76f, 0.86f, 1.0f});
				const Vector4 secondShade = GetColorProperty(*master, "secondShadeColor", {0.42f, 0.46f, 0.58f, 1.0f});
				surface.baseColor = ColorInputExpression(material, *master, "Base Color", useMaterialCBufferDefaults ? surface.baseColor : ColorExpression(baseColor));
				surface.emissiveColor = ColorInputExpression(material, *master, "Emissive", useMaterialCBufferDefaults ? surface.emissiveColor : ColorExpression(emissiveColor));
				surface.emissiveIntensity = FloatInputExpression(material, *master, "Emissive Intensity", useMaterialCBufferDefaults ? surface.emissiveIntensity : FloatExpression(GetFloatProperty(*master, "emissiveIntensity", material.emissiveIntensity)));
				surface.toonBaseColor = useMaterialCBufferDefaults ? surface.toonBaseColor : ColorExpression({1, 1, 1, 1});
				surface.toonHighlightColor = ColorInputExpression(material, *master, "Highlight", useMaterialCBufferDefaults ? surface.toonHighlightColor : ColorExpression(highlightColor));
				surface.toonFirstShadeColor = ColorInputExpression(material, *master, "1st Shade", useMaterialCBufferDefaults ? surface.toonFirstShadeColor : ColorExpression(firstShade));
				surface.toonSecondShadeColor = ColorInputExpression(material, *master, "2nd Shade", useMaterialCBufferDefaults ? surface.toonSecondShadeColor : ColorExpression(secondShade));
				surface.toonBaseStep = FloatInputExpression(material, *master, "Base Step", useMaterialCBufferDefaults ? surface.toonBaseStep : FloatExpression(GetFloatProperty(*master, "baseStep", 0.25f)));
				surface.toonBaseFeather = FloatInputExpression(material, *master, "Base Feather", useMaterialCBufferDefaults ? surface.toonBaseFeather : FloatExpression(GetFloatProperty(*master, "baseFeather", 0.03f)));
				surface.toonShadeStep = FloatInputExpression(material, *master, "Shade Step", useMaterialCBufferDefaults ? surface.toonShadeStep : FloatExpression(GetFloatProperty(*master, "shadeStep", -0.15f)));
				surface.toonShadeFeather = FloatInputExpression(material, *master, "Shade Feather", useMaterialCBufferDefaults ? surface.toonShadeFeather : FloatExpression(GetFloatProperty(*master, "shadeFeather", 0.03f)));
				surface.toonSpecularThreshold = FloatInputExpression(material, *master, "Spec Threshold", useMaterialCBufferDefaults ? surface.toonSpecularThreshold : FloatExpression(GetFloatProperty(*master, "specularThreshold", 0.96f)));
				surface.toonSpecularSoftness = FloatInputExpression(material, *master, "Spec Softness", useMaterialCBufferDefaults ? surface.toonSpecularSoftness : FloatExpression(GetFloatProperty(*master, "specularSoftness", 0.02f)));
				surface.toonSpecularIntensity = FloatInputExpression(material, *master, "Spec Intensity", useMaterialCBufferDefaults ? surface.toonSpecularIntensity : FloatExpression(GetFloatProperty(*master, "specularIntensity", 0.35f)));
				surface.normalMap = ColorInputExpression(material, *master, "Normal Map", surface.normalMap);
				surface.normalMapStrength = FloatInputExpression(material, *master, "Normal Strength", surface.normalMapStrength);
				return surface;
			}

			if(master->type == "LitMaster") {
				surface.lightingMode = static_cast<int32_t>(GetFloatProperty(*master, "lightingMode", 0.0f));
				surface.baseColor = ColorInputExpression(material, *master, "Base Color", useMaterialCBufferDefaults ? surface.baseColor : ColorExpression(material.color));
				surface.emissiveColor = ColorInputExpression(material, *master, "Emissive", useMaterialCBufferDefaults ? surface.emissiveColor : ColorExpression(GetColorProperty(*master, "emissiveColor", material.emissiveColor)));
				surface.emissiveIntensity = FloatInputExpression(material, *master, "Emissive Intensity", useMaterialCBufferDefaults ? surface.emissiveIntensity : FloatExpression(GetFloatProperty(*master, "emissiveIntensity", material.emissiveIntensity)));
				surface.shininess = FloatInputExpression(material, *master, "Shininess", useMaterialCBufferDefaults ? surface.shininess : FloatExpression(GetFloatProperty(*master, "shininess", material.shininess)));
				surface.roughness = FloatInputExpression(material, *master, "Roughness", useMaterialCBufferDefaults ? surface.roughness : FloatExpression(GetFloatProperty(*master, "roughness", material.roughness)));
				surface.normalMap = ColorInputExpression(material, *master, "Normal Map", surface.normalMap);
				surface.normalMapStrength = FloatInputExpression(material, *master, "Normal Strength", surface.normalMapStrength);
				return surface;
			}

			if(master->type == "UnlitMaster") {
				surface.lightingMode = 4;
				surface.baseColor = ColorInputExpression(material, *master, "Base Color", useMaterialCBufferDefaults ? surface.baseColor : ColorExpression(material.color));
				surface.emissiveColor = ColorInputExpression(material, *master, "Emissive", useMaterialCBufferDefaults ? surface.emissiveColor : ColorExpression(GetColorProperty(*master, "emissiveColor", material.emissiveColor)));
				surface.emissiveIntensity = FloatInputExpression(material, *master, "Emissive Intensity", useMaterialCBufferDefaults ? surface.emissiveIntensity : FloatExpression(GetFloatProperty(*master, "emissiveIntensity", material.emissiveIntensity)));
				return surface;
			}

			ApplyLegacyOutputExpressions(material, *output, surface);
			return surface;
		}
	};
}
