#include "../Copy/CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
	PixelShaderOutput output;
	output.color = gTexture.Sample(gSampler, input.texcoord);
	float value = dot(output.color.rgb, float3(0.2125f, 0.7154f, 0.0721f));
    
	output.color.a = 1.0f;
	output.color.rgb = float3(value, value, value);
	return output;
}