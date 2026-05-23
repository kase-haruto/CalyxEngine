#include "Object2d.hlsli"

struct VSInput {
	float4 position : POSITION;
	float2 texcoord : TEXCOORD0;
};

cbuffer VSConstants : register(b0) {
	TransformationMatrix gTransformationMat;
}

VSOutput main(VSInput input) {
	VSOutput output;
    
	output.position = mul(input.position, gTransformationMat.WVP);
	output.texcoord = input.texcoord;

	return output;
}