#pragma once
struct VertexShaderOutput {
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL0;
	float3 worldPosition : POSITION0;
};

cbuffer CameraConstants : register(b1) {
	float4x4 View;
	float4x4 Projection;
	float4x4 ViewProjection;
	float3 cameraPosition;
	float3 camRight;
	float3 camUp;
	float3 camForward;

	float2 viewportSize;
}