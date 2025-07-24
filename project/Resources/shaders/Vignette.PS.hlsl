#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
	PixelShaderOutput output;
	output.color = gTexture.Sample(gSampler, input.texcoord);

	//周囲を0に、中心になるほど明るくなるように計算で調整
	float2 correct = input.texcoord * (1.0f - input.texcoord.yx);
	
	float vignette = correct.x * correct.y * 16.0f;
	
	//乗さん
	output.color.rgb *= vignette;

	return output;
}