#include "Object2d.hlsli"

struct PSOutput {
	float4 color : SV_TARGET0;
};

struct Material {
	float4   color;
	float4x4 uvTransform;

	float  fillAmount;
	float2 fillOrigin;
	int    fillMethod;
};

ConstantBuffer<Material> gMaterial : register(b0);

Texture2D<float4> gTexture : register(t0);
//Texture2D<float4> gMaskTexture : register(t1); // mask mode時に使用
SamplerState      gSampler : register(s0);

PSOutput main(VSOutput input) {
	PSOutput output;

	float2 baseUV = input.texcoord; // フィル判定用（動かさない）

	// フィル判定
	if (gMaterial.fillMethod == 1) {
		if (gMaterial.fillOrigin.x < 0.5f) {
			if (baseUV.x > gMaterial.fillAmount) discard;
		} else {
			if (baseUV.x < 1.0f - gMaterial.fillAmount) discard;
		}
	}

	// --------------------------
	// こちらはアニメ用UV
	// fillAmount の判定に使わない
	// --------------------------
	float4 uvT   = mul(float4(baseUV,0,1), gMaterial.uvTransform);
	float2 animUV = uvT.xy;

	float4 texColor = gTexture.Sample(gSampler, animUV);

	output.color = texColor * gMaterial.color;
	return output;
}