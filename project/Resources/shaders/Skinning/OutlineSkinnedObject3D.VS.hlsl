#include "../Core/Object3D.hlsli"

struct Well {
	float4x4 skeletonSpaceMatrix;
	float4x4 skeletonSpaceInverseTransposeMatrix;
};

struct Skinned {
	float4 position;
	float3 normal;
};

struct VertexShaderInput {
	float4 position : POSITION0;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL0;
};

cbuffer ObjectConstants : register(b0) {
	float4x4 World;
	float4x4 WorldInverseTranspose;
}

cbuffer OutlineConstants : register(b2) {
	float4 outlineColor;
	float outlineThickness;
}

VertexShaderOutput main(VertexShaderInput input) {
	VertexShaderOutput output;

	float4 worldPos = mul(input.position, World);
	float3 nWorld = normalize(mul(input.normal, (float3x3)WorldInverseTranspose));
	worldPos.xyz += nWorld * outlineThickness;

	output.position = mul(worldPos, ViewProjection);
	output.worldPosition = worldPos.xyz;
	output.texcoord = input.texcoord;
	output.normal = nWorld;
	return output;
}
