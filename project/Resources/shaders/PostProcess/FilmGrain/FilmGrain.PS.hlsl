#include "../Copy/CopyImage.hlsli"

cbuffer FilmGrainParam : register(b0) {
	float intensity;
	float grainSize;
	float time;
	float speed;
};

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

float Hash(float3 p) {
	p = frac(p * 0.1031f);
	p += dot(p, p.yzx + 33.33f);
	return frac((p.x + p.y) * p.z);
}

float4 main(VertexShaderOutput input) : SV_TARGET0 {
	float4 color = gTexture.Sample(gSampler, input.texcoord);
	uint width, height;
	gTexture.GetDimensions(width, height);
	float2 pixel = floor(input.texcoord * float2(width, height) / max(grainSize, 0.5f));
	float noise = Hash(float3(pixel, floor(time * 60.0f))) * 2.0f - 1.0f;
	// Scale noise slightly by luminance so highlights do not clip as readily.
	float luminance = dot(color.rgb, float3(0.2126f, 0.7152f, 0.0722f));
	color.rgb += noise * intensity * (0.65f + 0.35f * (1.0f - luminance));
	return color;
}
