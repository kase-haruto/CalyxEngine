#include "../BackgroundTypes.hlsli"
#include "../Common/NoiseCommon.hlsli"

cbuffer DustParameters : register(b1) {
    float gDustDensity;
    float gDustSize;
    float gDustSpeed;
    float gDustAlpha;
    float gDustParallaxStrength;
    float gDustTime;
    float2 gDustPadding;
};

BackgroundOutput main(BackgroundVSOutput input) {
    float aspect = gViewportSize.x / max(gViewportSize.y, 1.0f);
    float2 p = float2(input.uv.x * aspect, input.uv.y) * 34.0f;
    p += gCameraPosition.xy * gDustParallaxStrength * 0.03f;
    p += float2(gDustTime * gDustSpeed * 0.19f, -gDustTime * gDustSpeed * 0.07f);
    float2 cell = floor(p);
    float2 local = frac(p) - 0.5f;
    float3 random = Hash33(float3(cell, 73.0f));
    float exists = step(1.0f - saturate(gDustDensity), random.x);
    float2 center = (random.yz - 0.5f) * 0.75f;
    float radius = gDustSize * lerp(0.45f, 1.5f, random.y);
    float particle = exp2(-dot(local-center,local-center) / max(radius*radius, 1e-5f) * 2.0f) * exists;
    float3 tint = lerp(float3(0.25f,0.55f,1.0f), float3(0.9f,0.35f,0.85f), random.z);
    float3 color = tint * particle * gDustAlpha;
    BackgroundOutput output;
    output.color = float4(color, particle * gDustAlpha);
    output.bloomMask = 0.0f;
    return output;
}
