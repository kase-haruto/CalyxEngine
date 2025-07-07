#include "Object3D.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////
struct VertexShaderInput {
	float4 position : POSITION0;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL0;
};

struct TransformationMat {
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

///////////////////////////////////////////////////////////////////////////////
//                            tables
///////////////////////////////////////////////////////////////////////////////
StructuredBuffer<TransformationMat> gTransMat : register(t0);

///////////////////////////////////////////////////////////////////////////////
//                            main
///////////////////////////////////////////////////////////////////////////////
VertexShaderOutput main(VertexShaderInput input,uint instancedId : SV_InstanceID) {
	VertexShaderOutput output;

    float4 worldPos = mul(input.position, gTransMat[instancedId].World);
	output.position = mul(worldPos, ViewProjection);
	output.worldPosition = worldPos.xyz;

	output.texcoord = input.texcoord;
	output.normal = normalize(mul(input.normal, (float3x3)gTransMat[instancedId].WorldInverseTranspose)); // 法線変換

	return output;
}