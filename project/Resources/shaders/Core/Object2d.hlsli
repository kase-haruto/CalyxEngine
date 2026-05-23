struct VSOutput {
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};


struct TransformationMatrix {
	float4x4 WVP;
	float4x4 world;
	float4x4 worldInverseTranspose;
};