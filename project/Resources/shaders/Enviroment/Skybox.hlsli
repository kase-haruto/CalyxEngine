
struct VertexShaderOutput{
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD0;
};
   

cbuffer CameraConstants : register(b1){
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;
    float3 cameraPosition;
}