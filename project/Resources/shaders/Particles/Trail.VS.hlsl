#include "Trail.hlsli"

struct TrailVertexInput {
	float3 position : POSITION;
	float2 uv : TEXCOORD0;
	float4 color : COLOR0;
	float normalizedAge : TEXCOORD1;
	float normalizedLength : TEXCOORD2;
	float randomValue : TEXCOORD3;
	float emissiveIntensity : TEXCOORD4;
};

struct Camera {
	float4x4 view;
	float4x4 projection;
	float4x4 viewProjection;
	float3 cameraPosition;
	float3 camRight;
	float3 camUp;
	float3 camForward;
};

ConstantBuffer<Camera> gCamera : register(b0);

TrailVertexOutput main(TrailVertexInput input) {
	TrailVertexOutput output;
	// TrailMeshBuilderがWorld座標を生成するためWorld Matrixは適用しない。
	output.position = mul(float4(input.position,1.0f),gCamera.viewProjection);
	output.uv = input.uv;
	output.color = input.color;
	output.normalizedAge = input.normalizedAge;
	output.normalizedLength = input.normalizedLength;
	output.randomValue = input.randomValue;
	output.emissiveIntensity = input.emissiveIntensity;
	return output;
}
