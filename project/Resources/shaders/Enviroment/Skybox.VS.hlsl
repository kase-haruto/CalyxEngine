#include "Skybox.hlsli"

struct VertexShaderInput {
	float4 position : POSITION0;
};

cbuffer ObjectConstants : register(b0) {
	float4x4 World;
	float4x4 WorldInverseTranspose;
}

VertexShaderOutput main(VertexShaderInput input) {
	VertexShaderOutput output;

	float4 worldPos = mul(input.position, World);
	float4 clipPos = mul(worldPos, ViewProjection);
	clipPos.z = clipPos.w; // z = w にして奥行き最大
	output.position = clipPos;

    // テクスチャ座標をそのまま出力
	output.texcoord = normalize(input.position.xyz);
    
	return output;
}