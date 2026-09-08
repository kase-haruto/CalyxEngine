struct BackgroundVSOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

struct BackgroundOutput {
    float4 color : SV_TARGET0;
    float4 bloomMask : SV_TARGET1;
};

cbuffer BackgroundCamera : register(b0) {
    float4x4 gView;
    float4x4 gProjection;
    float4x4 gViewProjection;
    float3 gCameraPosition;
    float gCameraPadding0;
    float3 gCameraRight;
    float gCameraPadding1;
    float3 gCameraUp;
    float gCameraPadding2;
    float3 gCameraForward;
    float gCameraPadding3;
    float2 gViewportSize;
    uint gCameraDitherEnabled;
    float gCameraPadding4;
};

float3 MakeBackgroundViewRay(float2 uv) {
    float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
    float tanHalfFovX = rcp(max(abs(gProjection[0][0]), 0.0001f));
    float tanHalfFovY = rcp(max(abs(gProjection[1][1]), 0.0001f));
    return normalize(gCameraForward + gCameraRight * ndc.x * tanHalfFovX + gCameraUp * ndc.y * tanHalfFovY);
}
