///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////
struct VertexShaderOutput {
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float4 color : COLOR0;
	float fade : TEXCOORD1;
	float4 flipbookScaleOffset : TEXCOORD2;
	float4 emissiveColor : COLOR1;
	float emissiveIntensity : TEXCOORD3;
};
