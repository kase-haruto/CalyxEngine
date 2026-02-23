///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////
struct VertexShaderOutput{
	float4 position:SV_POSITION;
	float2 texcoord:TEXCOORD0;
	float3 worldPos:TEXCOORD1;
	float4 color:COLOR0;
};

struct Camera {
	float4x4 view;
	float4x4 projection;
	float4x4 viewProjection;
	float3	 cameraPosition;
	float3	 camRight;
	float3	 camUp;
	float3	 camForward;
};
ConstantBuffer<Camera>		  gCamera : register(b0);
