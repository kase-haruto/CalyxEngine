#include "Trail.hlsli"

struct TrailMaterial {
	float4 color;
	float4 baseTilingScroll;
	float4 noiseTilingScroll;
	float4 noiseDistortion;
	float4 dissolve;
	float4 dissolveEdge;
	float4 dissolveEdgeColor;
	float4 fadeClip;
	float4 emissiveColorIntensity;
};

struct TrailPixelOutput {
	float4 color : SV_TARGET0;
	float4 bloomMask : SV_TARGET1;
};

ConstantBuffer<TrailMaterial> gMaterial : register(b1);
Texture2D<float4> gBaseTexture : register(t0);
Texture2D<float4> gNoiseTexture : register(t1);
SamplerState gSampler : register(s0);

TrailPixelOutput main(TrailVertexOutput input) {
	float2 baseUv = input.uv * gMaterial.baseTilingScroll.xy
		+ gMaterial.baseTilingScroll.zw * gMaterial.noiseDistortion.w;
	float2 noiseUv = input.uv * gMaterial.noiseTilingScroll.xy
		+ gMaterial.noiseTilingScroll.zw * gMaterial.noiseDistortion.w
		+ input.randomValue.xx;
	float4 noiseSample = gMaterial.noiseDistortion.x > 0.5f
		? gNoiseTexture.Sample(gSampler,noiseUv) : float4(1,1,1,1);
	if(gMaterial.noiseDistortion.z > 0.000001f)
		baseUv += (noiseSample.rg * 2.0f - 1.0f) * gMaterial.noiseDistortion.z;
	float4 textureColor = gBaseTexture.Sample(gSampler,baseUv);
	float baseMask = textureColor.a;
	float noiseMask = lerp(1.0f,noiseSample.r,saturate(gMaterial.noiseDistortion.y));

	float dissolveMask = 1.0f;
	float edgeMask = 0.0f;
	if(gMaterial.dissolve.x > 0.5f) {
		float threshold = lerp(gMaterial.dissolve.y,gMaterial.dissolve.z,input.normalizedAge);
		float softness = max(gMaterial.dissolve.w,0.0001f);
		dissolveMask = smoothstep(threshold-softness,threshold+softness,noiseSample.r);
		float edgeThreshold = threshold + max(gMaterial.dissolveEdge.x,0.0f);
		float outer = smoothstep(edgeThreshold-softness,edgeThreshold+softness,noiseSample.r);
		edgeMask = saturate(outer-dissolveMask);
	}

	float tailFade = gMaterial.fadeClip.y <= 0.0f ? 1.0f
		: smoothstep(0.0f,gMaterial.fadeClip.y,input.normalizedLength);
	float headFade = gMaterial.fadeClip.x <= 0.0f ? 1.0f
		: 1.0f-smoothstep(1.0f-gMaterial.fadeClip.x,1.0f,input.normalizedLength);
	float alpha = baseMask * noiseMask * dissolveMask * tailFade * headFade
		* input.color.a * gMaterial.color.a;
	if(gMaterial.fadeClip.z > 0.0f) clip(alpha-gMaterial.fadeClip.z);

	float3 baseColor = textureColor.rgb * input.color.rgb * gMaterial.color.rgb;
	float emissiveIntensity = max(input.emissiveIntensity,0.0f);
	float3 emissive = baseColor * gMaterial.emissiveColorIntensity.rgb
		* gMaterial.emissiveColorIntensity.w * emissiveIntensity;
	emissive += gMaterial.dissolveEdgeColor.rgb * edgeMask * gMaterial.dissolveEdge.y;

	TrailPixelOutput output;
	output.color = float4(baseColor + emissive,alpha);
	output.bloomMask = float4(emissive * alpha,alpha);
	return output;
}
