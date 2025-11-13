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

	float4 uvT = mul(float4(input.texcoord,0.0f,1.0f),gMaterial.uvTransform);
	float2 uv  = uvT.xy;

	// ======================================
	// 汎用 Fill 処理
	// ======================================
	if(gMaterial.fillMethod == 1) {
		// Horizontal
		if(gMaterial.fillOrigin.x < 0.5f) {
			// 左 -> 右
			if(uv.x > gMaterial.fillAmount) discard;
		} else {
			// 右 -> 左
			if(uv.x < 1.0f - gMaterial.fillAmount) discard;
		}
	} else if(gMaterial.fillMethod == 2) {
		// Vertical
		if(gMaterial.fillOrigin.y < 0.5f) {
			// 下 -> 上
			if(uv.y > gMaterial.fillAmount) discard;
		} else {
			// 上 -> 下
			if(uv.y < 1.0f - gMaterial.fillAmount) discard;
		}
	} else if(gMaterial.fillMethod == 3) {
		// Mask Fill
		// float mask = gMaskTexture.Sample(gSampler,uv).r;
		// if(mask < gMaterial.fillAmount)
		// 	discard;
	}

	// ======================================
	// 通常スプライト処理
	// ======================================
	float4 texColor = gTexture.Sample(gSampler,uv);
	output.color    = float4(
		texColor.rgb * gMaterial.color.rgb,
		texColor.a * gMaterial.color.a
		);
	return output;
}