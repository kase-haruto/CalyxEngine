#include "../BackgroundTypes.hlsli"
#include "../Common/NoiseCommon.hlsli"

cbuffer NebulaParameters : register(b1) {
    float4 gPrimaryColor;
    float4 gSecondaryColor;
    float4 gAccentColor;
    float gNoiseScale;
    float gNoiseStrength;
    float gWarpScale;
    float gWarpStrength;
    float gAnimationSpeed;
    float gBrightness;
    float gEmission;
    float gParallaxStrength;
    float gTime;
    float3 gNebulaPadding;
};

BackgroundOutput main(BackgroundVSOutput input) {
    float3 ray = MakeBackgroundViewRay(input.uv);
    float3 parallax = gCameraPosition * gParallaxStrength * 0.001f;
    float3 p = ray * max(gNoiseScale, 0.01f) + parallax;

    // ワープ領域だけを動かし、星雲全体を一方向へ流さずに呼吸するような形状変化を作る。
    // 背景の移動が宇宙船の移動表現と競合しないようにする。
    float phase = gTime * gAnimationSpeed;
    float3 warpMotionA = float3(sin(phase * 0.73f), cos(phase * 0.51f), sin(phase * 0.37f)) * 0.42f;
    float3 warpMotionB = float3(cos(phase * 0.41f), sin(phase * 0.67f), cos(phase * 0.29f)) * 0.36f;
    float2 warp = float2(
        Fbm4(p * gWarpScale + 4.7f + warpMotionA),
        Fbm4(p * gWarpScale + 19.3f + warpMotionB)) - 0.5f;
    float3 breathingWarp = float3(warp, warp.x - warp.y) * gWarpStrength;
    float primary = Fbm4(p + breathingWarp);
    float secondary = Fbm4(p * 1.65f - float3(warp.y, warp.x, warp.y) * 0.65f + warpMotionB * 0.12f);
    float density = smoothstep(0.27f, 0.68f, primary * gNoiseStrength + secondary * 0.28f);
    float core = smoothstep(0.52f, 0.84f, primary + secondary * 0.18f);

    float gradient = saturate(primary * 0.75f + ray.y * 0.22f + 0.12f);
    float3 nebula = lerp(gPrimaryColor.rgb, gSecondaryColor.rgb, smoothstep(0.2f, 0.75f, gradient));
    nebula = lerp(nebula, gAccentColor.rgb, core * 0.38f);
    nebula *= density * gBrightness;
    float3 deepSpace = float3(0.007f, 0.011f, 0.028f) + float3(0.010f, 0.004f, 0.018f) * (0.5f + 0.5f * ray.y);

    BackgroundOutput output;
    output.color = float4(deepSpace + nebula, 1.0f);
    output.bloomMask = float4(nebula * min(gEmission, 0.22f), 1.0f);
    return output;
}
