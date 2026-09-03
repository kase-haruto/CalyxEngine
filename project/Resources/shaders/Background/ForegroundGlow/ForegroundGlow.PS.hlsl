#include "../BackgroundTypes.hlsli"

cbuffer GlowParameters : register(b1) {
    float4 gGlowColor0;
    float4 gGlowColor1;
    float4 gGlowColor2;
    float4 gGlowPositionRadius0;
    float4 gGlowPositionRadius1;
    float4 gGlowPositionRadius2;
    float4 gGlowMoveSpeedIntensity;
};

float Blob(float2 uv, float4 positionRadius, float2 offset) {
    float2 d = uv - positionRadius.xy - offset;
    d.x *= gViewportSize.x / max(gViewportSize.y, 1.0f);
    float r = max(positionRadius.z, 0.01f);
    return exp2(-dot(d,d) / (r*r) * 1.7f) * positionRadius.w;
}

BackgroundOutput main(BackgroundVSOutput input) {
    float t = gGlowMoveSpeedIntensity.x;
    float speed = gGlowMoveSpeedIntensity.y;
    float2 move = float2(sin(t * speed * 0.83f), cos(t * speed * 0.61f)) * 0.04f;
    float3 color = gGlowColor0.rgb * Blob(input.uv, gGlowPositionRadius0, move);
    color += gGlowColor1.rgb * Blob(input.uv, gGlowPositionRadius1, -move * 0.73f);
    color += gGlowColor2.rgb * Blob(input.uv, gGlowPositionRadius2, move.yx * 0.51f);
    color *= gGlowMoveSpeedIntensity.z;
    BackgroundOutput output;
    output.color = float4(color, saturate(max(max(color.r,color.g),color.b)));
    output.bloomMask = 0.0f;
    return output;
}
