#include "CopyImage.hlsli"

Texture2D<float> gDepth : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
	PixelShaderOutput output;

	const float depth = saturate(gDepth.Sample(gSampler, input.texcoord));
	const float occupied = 1.0f - step(0.99999f, depth);
	const float viewDepth = saturate((1.0f - depth) * 100.0f) * occupied;
	const float grid = frac(input.texcoord.x * 20.0f) < 0.01f || frac(input.texcoord.y * 20.0f) < 0.01f ? 0.08f : 0.0f;
	const float value = max(viewDepth, grid * (1.0f - occupied));

	output.color = float4(value.xxx, 1.0f);
	return output;
}
