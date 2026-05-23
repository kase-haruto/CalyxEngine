#include "Fragment.hlsli"

cbuffer ObjectConstants : register(b0) {
	float4x4 World;
}

cbuffer CameraConstants : register(b1) {
	float4x4 View;
	float4x4 Projection;
	float4x4 ViewProjection;
	float3 cameraPosition;
};

VSOutput main(float4 pos : POSITION, float4 color : COLOR) {
	VSOutput output; // ピクセルシェーダーに渡す値
	float4 worldPos = mul(pos, World);
	output.svpos = mul(worldPos, ViewProjection);
	output.color = color;

	return output;
}