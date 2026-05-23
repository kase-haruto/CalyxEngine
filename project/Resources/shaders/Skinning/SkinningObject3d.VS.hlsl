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

struct TransformationMat {
	float4x4 World;
	float4x4 WorldInverseTranspose;
};

StructuredBuffer<Well> gMatrixPalette : register(t0);
StructuredBuffer<TransformationMat> gTransMat : register(t1);

VertexShaderOutput main(VertexShaderInput input, uint instancedId : SV_InstanceID) {

	VertexShaderOutput output;
	
	const TransformationMat tm = gTransMat[instancedId];
	
	float4 worldPos = mul(input.position, tm.World);
	output.position = mul(worldPos, ViewProjection);
	output.worldPosition = worldPos.xyz;
	output.texcoord = input.texcoord;
	output.normal = normalize(mul(input.normal, (float3x3)tm.WorldInverseTranspose));
	
	return output;
}