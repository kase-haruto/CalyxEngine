#include "Object3D.hlsli"

float4 main(VertexShaderOutput input) : SV_TARGET {
	float3 n = normalize(input.normal);
	return float4(n * 0.5f + 0.5f, input.position.z);
}
