#include "Text.hlsli"

struct VSInput {
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

cbuffer TextViewport : register(b0) {
    float2 gViewportSize;
};

VSOutput main(VSInput input) {
    VSOutput output;
    float2 ndc;
    ndc.x = input.position.x / gViewportSize.x * 2.0f - 1.0f;
    ndc.y = 1.0f - input.position.y / gViewportSize.y * 2.0f;
    output.position = float4(ndc, 0.0f, 1.0f);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}
