///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////
#include "CopyImage.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////
struct RadialBulurData {
	float2 center;
	float width;
};

///////////////////////////////////////////////////////////////////////////////
//                            cbuffers
///////////////////////////////////////////////////////////////////////////////
cbuffer RadialBulurData : register(b0) {
	RadialBulurData gBulurData;
}

///////////////////////////////////////////////////////////////////////////////
//                            tables
///////////////////////////////////////////////////////////////////////////////
Texture2D<float4> gTexture : register(t0);

///////////////////////////////////////////////////////////////////////////////
//                            samplers
///////////////////////////////////////////////////////////////////////////////
SamplerState gSampler : register(s0);

float4 main(VertexShaderOutput input) : SV_TARGET {
	const int kNumSamples = 10;


	float2 dir = input.texcoord - gBulurData.center;
	float2 step = dir * gBulurData.width / kNumSamples;

	float3 accumColor = float3(0.0f, 0.0f, 0.0f);
	float totalWeight = 0.0f;

	for(int i = 0; i < kNumSamples; ++i) {
		float2 sampleUV = input.texcoord - step * i;
		float weight = 1.0f - (float)i / kNumSamples;
		accumColor += gTexture.Sample(gSampler, sampleUV).rgb * weight;
		totalWeight += weight;
	}

	accumColor /= totalWeight;

	return float4(accumColor, 1.0f);
}