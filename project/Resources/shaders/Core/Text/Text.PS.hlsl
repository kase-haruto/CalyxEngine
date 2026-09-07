#include "Text.hlsli"

Texture2D<float> gGlyphAtlas : register(t0);
SamplerState gSampler : register(s0);

float4 main(VSOutput input) : SV_TARGET0 {
    const float coverage = gGlyphAtlas.Sample(gSampler, input.uv);
    return float4(input.color.rgb, input.color.a * coverage);
}
