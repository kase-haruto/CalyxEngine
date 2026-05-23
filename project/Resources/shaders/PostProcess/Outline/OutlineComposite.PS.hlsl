#include "../Copy/CopyImage.hlsli"

Texture2D<float4> gColor : register(t0);
Texture2D<float> gDepth : register(t1);
Texture2D<float4> gNormal : register(t2);
SamplerState gSampler : register(s0);

cbuffer OutlineCompositeConstants : register(b0) {
	float4 outlineColor;
	float2 texelSize;
	float depthThreshold;
	float normalThreshold;
	float depthScale;
	float normalScale;
	float thickness;
	float insideMaskOnly;
	float4 padding;
}

float MaskFromDepth(float objectDepth) {
	return step(1e-6f, objectDepth);
}

float NormalEdge(float3 centerNormal, float centerMask, float3 sampleNormal, float sampleMask) {
	float valid = saturate(centerMask + sampleMask);
	float d = length(centerNormal - sampleNormal) * normalScale;
	return smoothstep(normalThreshold, 1.0f, d) * valid;
}

float DepthEdge(float centerObjectDepth, float centerMask, float sampleObjectDepth, float sampleMask) {
	float valid = saturate(centerMask + sampleMask);
	if(centerMask < 0.5f || sampleMask < 0.5f) {
		return valid;
	}
	float nearDepth = max(min(centerObjectDepth, sampleObjectDepth), 1e-5f);
	float d = abs(centerObjectDepth - sampleObjectDepth) / nearDepth * depthScale;
	return smoothstep(depthThreshold, 1.0f, d) * valid;
}

float4 main(VertexShaderOutput input) : SV_TARGET {
	const float2 uv = input.texcoord;
	const float4 baseColor = gColor.Sample(gSampler, uv);
	const float centerDepth = gDepth.Sample(gSampler, uv);
	const float4 centerNormalTex = gNormal.Sample(gSampler, uv);
	const float3 centerNormal = centerNormalTex.xyz * 2.0f - 1.0f;
	const float centerObjectDepth = centerNormalTex.a;
	const float centerMask = MaskFromDepth(centerObjectDepth);

	float edge = 0.0f;
	const int radius = clamp((int)round(thickness), 1, 4);

	[loop]
	for(int y = -radius; y <= radius; ++y) {
		[loop]
		for(int x = -radius; x <= radius; ++x) {
			if(x == 0 && y == 0) continue;
			float2 o = float2((float)x, (float)y) * texelSize;
			float4 snTex = gNormal.Sample(gSampler, uv + o);
			float3 sn = snTex.xyz * 2.0f - 1.0f;
			float sod = snTex.a;
			float sm = MaskFromDepth(sod);
			edge = max(edge, DepthEdge(centerObjectDepth, centerMask, sod, sm));
			edge = max(edge, NormalEdge(centerNormal, centerMask, sn, sm));
		}
	}

	edge = saturate(edge);
	if(insideMaskOnly > 0.5f) {
		const float visibleSelected = step(centerObjectDepth, centerDepth + 0.0005f);
		edge *= centerMask * visibleSelected;
	}
	return lerp(baseColor, outlineColor, edge * outlineColor.a);
}
