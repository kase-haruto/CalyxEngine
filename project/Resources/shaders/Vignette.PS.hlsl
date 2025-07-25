#include "CopyImage.hlsli"

cbuffer VignetteParam : register(b0) {
	float strength;
	float radius;
};

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
	PixelShaderOutput output;

	float2 uv = input.texcoord;

    // テクスチャサンプリング
	output.color = gTexture.Sample(gSampler, uv);

    // ビネット計算
	float2 toCenter = uv - float2(0.5f, 0.5f);
	float dist = length(toCenter);

    // radius = 暗くなり始める位置（0.0〜1.0）
    // strength = 暗さの強度（0.0〜1.0）
	float vignetteFactor = 1.0f - smoothstep(radius, 1.0f, dist);
	vignetteFactor = lerp(1.0f, vignetteFactor, strength); // strength=1で完全に暗くなる

    // ビネット乗算
	output.color.rgb *= vignetteFactor;

	return output;
}
