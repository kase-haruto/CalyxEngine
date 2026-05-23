#include "../Copy/CopyImage.hlsli"

Texture2D<float4> gBaseTexture : register(t0);
Texture2D<float4> gBlendTexture : register(t1);
SamplerState gSampler : register(s0);

cbuffer BlendParameter : register(b0) {
	float opacity;
	float mode;
	float2 padding;
}

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

float3 ApplyBlend(float3 a, float3 b, float modeValue) {
	if(modeValue < 0.5f) {
		return lerp(a, b, opacity);
	}
	if(modeValue < 1.5f) {
		return a + b * opacity;
	}
	if(modeValue < 2.5f) {
		return a * lerp(float3(1.0f, 1.0f, 1.0f), b, opacity);
	}
	return max(a, b * opacity);
}

PixelShaderOutput main(VertexShaderOutput input) {
	PixelShaderOutput output;
	float4 baseColor = gBaseTexture.Sample(gSampler, input.texcoord);
	float4 blendColor = gBlendTexture.Sample(gSampler, input.texcoord);
	output.color.rgb = saturate(ApplyBlend(baseColor.rgb, blendColor.rgb, mode));
	output.color.a = 1.0f;
	return output;
}
