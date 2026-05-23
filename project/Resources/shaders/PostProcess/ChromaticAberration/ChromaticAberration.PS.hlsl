#include "../Copy/CopyImage.hlsli"

///////////////////////////////////////////////////////////////////////////////
//                            structs
///////////////////////////////////////////////////////////////////////////////
struct ChromaticAberrationData {
    float intensity;
};

///////////////////////////////////////////////////////////////////////////////
//                            cbuffers
///////////////////////////////////////////////////////////////////////////////
cbuffer ChromaticAberrationData : register(b0) {
    ChromaticAberrationData gChromaticAberration;
}

///////////////////////////////////////////////////////////////////////////////
//                            tables
///////////////////////////////////////////////////////////////////////////////
Texture2D<float4> gTexture : register(t0);

///////////////////////////////////////////////////////////////////////////////
//                            samplers
///////////////////////////////////////////////////////////////////////////////
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input){
    PixelShaderOutput output;

    // UV座標
    float2 uv = input.texcoord;

    // 画面中心からの方向ベクトル
    float2 center = float2(0.5, 0.5);
    float2 direction = uv - center;
    float dist = length(direction);

    // 強さを距離で調整して自然な収差に
    float2 dynamicOffset = gChromaticAberration.intensity * direction * dist;

    // R, G, B 成分をそれぞれオフセットしてサンプリング
    float3 color;
    color.r = gTexture.Sample(gSampler, uv + dynamicOffset).r;
    color.g = gTexture.Sample(gSampler, uv).g;
    color.b = gTexture.Sample(gSampler, uv - dynamicOffset).b;

    output.color = float4(color, 1.0f);

    return output;
}