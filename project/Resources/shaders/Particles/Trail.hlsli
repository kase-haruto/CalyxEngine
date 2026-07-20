struct TrailVertexOutput {
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float4 color : COLOR0;
	float normalizedAge : TEXCOORD1;
	float normalizedLength : TEXCOORD2;
	float randomValue : TEXCOORD3;
	float emissiveIntensity : TEXCOORD4;
};
